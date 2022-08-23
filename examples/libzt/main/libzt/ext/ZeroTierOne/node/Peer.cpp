/*
 * Copyright (c)2013-2020 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2025-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

#include "../version.h"
#include "Constants.hpp"
#include "Peer.hpp"
#include "Switch.hpp"
#include "Network.hpp"
#include "SelfAwareness.hpp"
#include "Packet.hpp"
#include "Trace.hpp"
#include "InetAddress.hpp"
#include "RingBuffer.hpp"
#include "Utils.hpp"

namespace ZeroTier {

static unsigned char s_freeRandomByteCounter = 0;

Peer::Peer(const RuntimeEnvironment *renv,const Identity &myIdentity,const Identity &peerIdentity) :
	RR(renv),
	_lastReceive(0),
	_lastNontrivialReceive(0),
	_lastTriedMemorizedPath(0),
	_lastDirectPathPushSent(0),
	_lastDirectPathPushReceive(0),
	_lastCredentialRequestSent(0),
	_lastWhoisRequestReceived(0),
	_lastCredentialsReceived(0),
	_lastTrustEstablishedPacketReceived(0),
	_lastSentFullHello(0),
	_lastEchoCheck(0),
	_freeRandomByte((unsigned char)((uintptr_t)this >> 4) ^ ++s_freeRandomByteCounter),
	_vProto(0),
	_vMajor(0),
	_vMinor(0),
	_vRevision(0),
	_id(peerIdentity),
	_directPathPushCutoffCount(0),
	_echoRequestCutoffCount(0),
	_localMultipathSupported(false),
	_lastComputedAggregateMeanLatency(0)
{
	if (!myIdentity.agree(peerIdentity,_key))
		throw ZT_EXCEPTION_INVALID_ARGUMENT;

	uint8_t ktmp[ZT_SYMMETRIC_KEY_SIZE];
	KBKDFHMACSHA384(_key,ZT_KBKDF_LABEL_AES_GMAC_SIV_K0,0,0,ktmp);
	_aesKeys[0].init(ktmp);
	KBKDFHMACSHA384(_key,ZT_KBKDF_LABEL_AES_GMAC_SIV_K1,0,0,ktmp);
	_aesKeys[1].init(ktmp);
	Utils::burn(ktmp,ZT_SYMMETRIC_KEY_SIZE);
}

void Peer::received(
	void *tPtr,
	const SharedPtr<Path> &path,
	const unsigned int hops,
	const uint64_t packetId,
	const unsigned int payloadLength,
	const Packet::Verb verb,
	const uint64_t inRePacketId,
	const Packet::Verb inReVerb,
	const bool trustEstablished,
	const uint64_t networkId,
	const int32_t flowId)
{
	const int64_t now = RR->node->now();

	_lastReceive = now;
	switch (verb) {
		case Packet::VERB_FRAME:
		case Packet::VERB_EXT_FRAME:
		case Packet::VERB_NETWORK_CONFIG_REQUEST:
		case Packet::VERB_NETWORK_CONFIG:
		case Packet::VERB_MULTICAST_FRAME:
			_lastNontrivialReceive = now;
			break;
		default:
			break;
	}

	recordIncomingPacket(path, packetId, payloadLength, verb, flowId, now);

	if (trustEstablished) {
		_lastTrustEstablishedPacketReceived = now;
		path->trustedPacketReceived(now);
	}

	if (hops == 0) {
		// If this is a direct packet (no hops), update existing paths or learn new ones
		bool havePath = false;
		{
			Mutex::Lock _l(_paths_m);
			for(unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
				if (_paths[i].p) {
					if (_paths[i].p == path) {
						_paths[i].lr = now;
						havePath = true;
						break;
					}
				} else {
					break;
				}
			}
		}

		if ( (!havePath) && RR->node->shouldUsePathForZeroTierTraffic(tPtr,_id.address(),path->localSocket(),path->address()) ) {

			/**
			 * First, fill all free slots before attempting to replace a path
			 *  - If the above fails, attempt to replace the path that has been dead the longest
			 *  - If there are no free slots, and no dead paths (unlikely), then replace old path most similar to new path
			 *  - If all of the above fails to yield a suitable replacement. Replace first path found to have lower `(quality / priority)`
			 */

			if (verb == Packet::VERB_OK) {
				Mutex::Lock _l(_paths_m);
				unsigned int replacePath = ZT_MAX_PEER_NETWORK_PATHS;
				uint64_t maxScore = 0;
				uint64_t currScore;
				long replacePathQuality = 0;
				bool foundFreeSlot = false;

				for(unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
					currScore = 0;
					if (_paths[i].p) {
						// Reward dead paths
						if (!_paths[i].p->alive(now)) {
							currScore = _paths[i].p->age(now) / 1000;
						}
						// Reward as similarity increases
						if (_paths[i].p->address().ipsEqual(path->address())) {
							currScore++;
							if (_paths[i].p->address().port() == path->address().port()) {
								currScore++;
								if (_paths[i].p->localSocket() == path->localSocket()) {
									currScore++; // max score (3)
								}
							}
						}
						// If best so far, mark for replacement
						if (currScore > maxScore) {
							maxScore = currScore;
							replacePath = i;
						}
					}
					else {
						foundFreeSlot = true;
						replacePath = i;
						break;
					}
				}
				if (!foundFreeSlot) {
					if (maxScore > 3) {
						// Do nothing. We found a dead path and have already marked it as a candidate
					}
					// If we couldn't find a replacement by matching, replacing a dead path, or taking a free slot, then replace by quality
					else if (maxScore == 0) {
						for(unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
							if (_paths[i].p) {
								const long q = _paths[i].p->quality(now) / _paths[i].priority;
								if (q > replacePathQuality) {
									replacePathQuality = q;
									replacePath = i;
								}
							}
						}
					}
				}

				if (replacePath != ZT_MAX_PEER_NETWORK_PATHS) {
					RR->t->peerLearnedNewPath(tPtr, networkId, *this, path, packetId);
					_paths[replacePath].lr = now;
					_paths[replacePath].p = path;
					_paths[replacePath].priority = 1;
					Mutex::Lock _l(_bond_m);
					if(_bond) {
						_bond->nominatePathToBond(_paths[replacePath].p, now);
					}
				}
			} else {
				Mutex::Lock ltl(_lastTriedPath_m);

				bool triedTooRecently = false;
				for(std::list< std::pair< Path *, int64_t > >::iterator i(_lastTriedPath.begin());i!=_lastTriedPath.end();) {
					if ((now - i->second) > 1000) {
						_lastTriedPath.erase(i++);
					} else if (i->first == path.ptr()) {
						++i;
						triedTooRecently = true;
					} else {
						++i;
					}
				}

				if (!triedTooRecently) {
					_lastTriedPath.push_back(std::pair< Path *, int64_t >(path.ptr(), now));
					attemptToContactAt(tPtr,path->localSocket(),path->address(),now,true);
					path->sent(now);
					RR->t->peerConfirmingUnknownPath(tPtr,networkId,*this,path,packetId,verb);
				}
			}
		}
	}

	// If we have a trust relationship periodically push a message enumerating
	// all known external addresses for ourselves. If we already have a path this
	// is done less frequently.
	if (this->trustEstablished(now)) {
		const int64_t sinceLastPush = now - _lastDirectPathPushSent;
		if (sinceLastPush >= ((hops == 0) ? ZT_DIRECT_PATH_PUSH_INTERVAL_HAVEPATH : ZT_DIRECT_PATH_PUSH_INTERVAL)) {
			_lastDirectPathPushSent = now;
			std::vector<InetAddress> pathsToPush(RR->node->directPaths());
			std::vector<InetAddress> ma = RR->sa->whoami();
			pathsToPush.insert(pathsToPush.end(), ma.begin(), ma.end());
			if (!pathsToPush.empty()) {
				std::vector<InetAddress>::const_iterator p(pathsToPush.begin());
				while (p != pathsToPush.end()) {
					Packet *const outp = new Packet(_id.address(),RR->identity.address(),Packet::VERB_PUSH_DIRECT_PATHS);
					outp->addSize(2); // leave room for count
					unsigned int count = 0;
					while ((p != pathsToPush.end())&&((outp->size() + 24) < 1200)) {
						uint8_t addressType = 4;
						switch(p->ss_family) {
							case AF_INET:
								break;
							case AF_INET6:
								addressType = 6;
								break;
							default: // we currently only push IP addresses
								++p;
								continue;
						}

						outp->append((uint8_t)0); // no flags
						outp->append((uint16_t)0); // no extensions
						outp->append(addressType);
						outp->append((uint8_t)((addressType == 4) ? 6 : 18));
						outp->append(p->rawIpData(),((addressType == 4) ? 4 : 16));
						outp->append((uint16_t)p->port());

						++count;
						++p;
					}
					if (count) {
						outp->setAt(ZT_PACKET_IDX_PAYLOAD,(uint16_t)count);
						outp->compress();
						outp->armor(_key,true,aesKeysIfSupported());
						path->send(RR,tPtr,outp->data(),outp->size(),now);
					}
					delete outp;
				}
			}
		}
	}
}

SharedPtr<Path> Peer::getAppropriatePath(int64_t now, bool includeExpired, int32_t flowId)
{
	Mutex::Lock _l(_paths_m);
	Mutex::Lock _lb(_bond_m);
	if (!_bond) {
		unsigned int bestPath = ZT_MAX_PEER_NETWORK_PATHS;
		/**
		 * Send traffic across the highest quality path only. This algorithm will still
		 * use the old path quality metric from protocol version 9.
		 */
		long bestPathQuality = 2147483647;
		for(unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
			if (_paths[i].p) {
				if ((includeExpired)||((now - _paths[i].lr) < ZT_PEER_PATH_EXPIRATION)) {
					const long q = _paths[i].p->quality(now) / _paths[i].priority;
					if (q <= bestPathQuality) {
						bestPathQuality = q;
						bestPath = i;
					}
				}
			} else break;
		}
		if (bestPath != ZT_MAX_PEER_NETWORK_PATHS) {
			return _paths[bestPath].p;
		}
		return SharedPtr<Path>();
	}
	return _bond->getAppropriatePath(now, flowId);
}

void Peer::introduce(void *const tPtr,const int64_t now,const SharedPtr<Peer> &other) const
{
	unsigned int myBestV4ByScope[ZT_INETADDRESS_MAX_SCOPE+1];
	unsigned int myBestV6ByScope[ZT_INETADDRESS_MAX_SCOPE+1];
	long myBestV4QualityByScope[ZT_INETADDRESS_MAX_SCOPE+1];
	long myBestV6QualityByScope[ZT_INETADDRESS_MAX_SCOPE+1];
	unsigned int theirBestV4ByScope[ZT_INETADDRESS_MAX_SCOPE+1];
	unsigned int theirBestV6ByScope[ZT_INETADDRESS_MAX_SCOPE+1];
	long theirBestV4QualityByScope[ZT_INETADDRESS_MAX_SCOPE+1];
	long theirBestV6QualityByScope[ZT_INETADDRESS_MAX_SCOPE+1];
	for(int i=0;i<=ZT_INETADDRESS_MAX_SCOPE;++i) {
		myBestV4ByScope[i] = ZT_MAX_PEER_NETWORK_PATHS;
		myBestV6ByScope[i] = ZT_MAX_PEER_NETWORK_PATHS;
		myBestV4QualityByScope[i] = 2147483647;
		myBestV6QualityByScope[i] = 2147483647;
		theirBestV4ByScope[i] = ZT_MAX_PEER_NETWORK_PATHS;
		theirBestV6ByScope[i] = ZT_MAX_PEER_NETWORK_PATHS;
		theirBestV4QualityByScope[i] = 2147483647;
		theirBestV6QualityByScope[i] = 2147483647;
	}

	Mutex::Lock _l1(_paths_m);

	for(unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
		if (_paths[i].p) {
			const long q = _paths[i].p->quality(now) / _paths[i].priority;
			const unsigned int s = (unsigned int)_paths[i].p->ipScope();
			switch(_paths[i].p->address().ss_family) {
				case AF_INET:
					if (q <= myBestV4QualityByScope[s]) {
						myBestV4QualityByScope[s] = q;
						myBestV4ByScope[s] = i;
					}
					break;
				case AF_INET6:
					if (q <= myBestV6QualityByScope[s]) {
						myBestV6QualityByScope[s] = q;
						myBestV6ByScope[s] = i;
					}
					break;
			}
		} else break;
	}

	Mutex::Lock _l2(other->_paths_m);

	for(unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
		if (other->_paths[i].p) {
			const long q = other->_paths[i].p->quality(now) / other->_paths[i].priority;
			const unsigned int s = (unsigned int)other->_paths[i].p->ipScope();
			switch(other->_paths[i].p->address().ss_family) {
				case AF_INET:
					if (q <= theirBestV4QualityByScope[s]) {
						theirBestV4QualityByScope[s] = q;
						theirBestV4ByScope[s] = i;
					}
					break;
				case AF_INET6:
					if (q <= theirBestV6QualityByScope[s]) {
						theirBestV6QualityByScope[s] = q;
						theirBestV6ByScope[s] = i;
					}
					break;
			}
		} else break;
	}

	unsigned int mine = ZT_MAX_PEER_NETWORK_PATHS;
	unsigned int theirs = ZT_MAX_PEER_NETWORK_PATHS;

	for(int s=ZT_INETADDRESS_MAX_SCOPE;s>=0;--s) {
		if ((myBestV6ByScope[s] != ZT_MAX_PEER_NETWORK_PATHS)&&(theirBestV6ByScope[s] != ZT_MAX_PEER_NETWORK_PATHS)) {
			mine = myBestV6ByScope[s];
			theirs = theirBestV6ByScope[s];
			break;
		}
		if ((myBestV4ByScope[s] != ZT_MAX_PEER_NETWORK_PATHS)&&(theirBestV4ByScope[s] != ZT_MAX_PEER_NETWORK_PATHS)) {
			mine = myBestV4ByScope[s];
			theirs = theirBestV4ByScope[s];
			break;
		}
	}

	if (mine != ZT_MAX_PEER_NETWORK_PATHS) {
		unsigned int alt = (unsigned int)RR->node->prng() & 1; // randomize which hint we send first for black magickal NAT-t reasons
		const unsigned int completed = alt + 2;
		while (alt != completed) {
			if ((alt & 1) == 0) {
				Packet outp(_id.address(),RR->identity.address(),Packet::VERB_RENDEZVOUS);
				outp.append((uint8_t)0);
				other->_id.address().appendTo(outp);
				outp.append((uint16_t)other->_paths[theirs].p->address().port());
				if (other->_paths[theirs].p->address().ss_family == AF_INET6) {
					outp.append((uint8_t)16);
					outp.append(other->_paths[theirs].p->address().rawIpData(),16);
				} else {
					outp.append((uint8_t)4);
					outp.append(other->_paths[theirs].p->address().rawIpData(),4);
				}
				outp.armor(_key,true,aesKeysIfSupported());
				_paths[mine].p->send(RR,tPtr,outp.data(),outp.size(),now);
			} else {
				Packet outp(other->_id.address(),RR->identity.address(),Packet::VERB_RENDEZVOUS);
				outp.append((uint8_t)0);
				_id.address().appendTo(outp);
				outp.append((uint16_t)_paths[mine].p->address().port());
				if (_paths[mine].p->address().ss_family == AF_INET6) {
					outp.append((uint8_t)16);
					outp.append(_paths[mine].p->address().rawIpData(),16);
				} else {
					outp.append((uint8_t)4);
					outp.append(_paths[mine].p->address().rawIpData(),4);
				}
				outp.armor(other->_key,true,other->aesKeysIfSupported());
				other->_paths[theirs].p->send(RR,tPtr,outp.data(),outp.size(),now);
			}
			++alt;
		}
	}
}

void Peer::sendHELLO(void *tPtr,const int64_t localSocket,const InetAddress &atAddress,int64_t now)
{
	Packet outp(_id.address(),RR->identity.address(),Packet::VERB_HELLO);

	outp.append((unsigned char)ZT_PROTO_VERSION);
	outp.append((unsigned char)ZEROTIER_ONE_VERSION_MAJOR);
	outp.append((unsigned char)ZEROTIER_ONE_VERSION_MINOR);
	outp.append((uint16_t)ZEROTIER_ONE_VERSION_REVISION);
	outp.append(now);
	RR->identity.serialize(outp,false);
	atAddress.serialize(outp);

	outp.append((uint64_t)RR->topology->planetWorldId());
	outp.append((uint64_t)RR->topology->planetWorldTimestamp());

	const unsigned int startCryptedPortionAt = outp.size();

	std::vector<World> moons(RR->topology->moons());
	std::vector<uint64_t> moonsWanted(RR->topology->moonsWanted());
	outp.append((uint16_t)(moons.size() + moonsWanted.size()));
	for(std::vector<World>::const_iterator m(moons.begin());m!=moons.end();++m) {
		outp.append((uint8_t)m->type());
		outp.append((uint64_t)m->id());
		outp.append((uint64_t)m->timestamp());
	}
	for(std::vector<uint64_t>::const_iterator m(moonsWanted.begin());m!=moonsWanted.end();++m) {
		outp.append((uint8_t)World::TYPE_MOON);
		outp.append(*m);
		outp.append((uint64_t)0);
	}

	outp.cryptField(_key,startCryptedPortionAt,outp.size() - startCryptedPortionAt);

	if (atAddress) {
		outp.armor(_key,false,nullptr); // false == don't encrypt full payload, but add MAC
		RR->node->expectReplyTo(outp.packetId());
		RR->node->putPacket(tPtr,-1,atAddress,outp.data(),outp.size());
	} else {
		RR->node->expectReplyTo(outp.packetId());
		RR->sw->send(tPtr,outp,false); // false == don't encrypt full payload, but add MAC
	}
}

void Peer::attemptToContactAt(void *tPtr,const int64_t localSocket,const InetAddress &atAddress,int64_t now,bool sendFullHello)
{
	if ( (!sendFullHello) && (_vProto >= 5) && (!((_vMajor == 1)&&(_vMinor == 1)&&(_vRevision == 0))) ) {
		Packet outp(_id.address(),RR->identity.address(),Packet::VERB_ECHO);
		outp.armor(_key,true,aesKeysIfSupported());
		RR->node->expectReplyTo(outp.packetId());
		RR->node->putPacket(tPtr,localSocket,atAddress,outp.data(),outp.size());
	} else {
		sendHELLO(tPtr,localSocket,atAddress,now);
	}
}

void Peer::tryMemorizedPath(void *tPtr,int64_t now)
{
	if ((now - _lastTriedMemorizedPath) >= ZT_TRY_MEMORIZED_PATH_INTERVAL) {
		_lastTriedMemorizedPath = now;
		InetAddress mp;
		if (RR->node->externalPathLookup(tPtr,_id.address(),-1,mp))
			attemptToContactAt(tPtr,-1,mp,now,true);
	}
}

void Peer::performMultipathStateCheck(void *tPtr, int64_t now)
{
	Mutex::Lock _l(_bond_m);
	if (_bond) {
		// Once enabled the Bond object persists, no need to update state
		return;
	}
	/**
	 * Check for conditions required for multipath bonding and create a bond
	 * if allowed.
	 */
	int numAlivePaths = 0;
	for(unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
		if (_paths[i].p && _paths[i].p->alive(now)) {
			numAlivePaths++;
		}
	}
	_localMultipathSupported = ((numAlivePaths >= 1) && (RR->bc->inUse()) && (ZT_PROTO_VERSION > 9));
	if (_localMultipathSupported && !_bond) {
		if (RR->bc) {
			_bond = RR->bc->createBond(RR, this);
			/**
			 * Allow new bond to retroactively learn all paths known to this peer
			 */
			if (_bond) {
				for (unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
					if (_paths[i].p) {
						_bond->nominatePathToBond(_paths[i].p, now);
					}
				}
			}
		}
	}
}

unsigned int Peer::doPingAndKeepalive(void *tPtr,int64_t now)
{
	unsigned int sent = 0;
	Mutex::Lock _l(_paths_m);

	performMultipathStateCheck(tPtr, now);

	const bool sendFullHello = ((now - _lastSentFullHello) >= ZT_PEER_PING_PERIOD);
	_lastSentFullHello = now;

	// Right now we only keep pinging links that have the maximum priority. The
	// priority is used to track cluster redirections, meaning that when a cluster
	// redirects us its redirect target links override all other links and we
	// let those old links expire.
	long maxPriority = 0;
	for(unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
		if (_paths[i].p)
			maxPriority = std::max(_paths[i].priority,maxPriority);
		else break;
	}

	unsigned int j = 0;
	for(unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
		if (_paths[i].p) {
			// Clean expired and reduced priority paths
			if ( ((now - _paths[i].lr) < ZT_PEER_PATH_EXPIRATION) && (_paths[i].priority == maxPriority) ) {
				if ((sendFullHello)||(_paths[i].p->needsHeartbeat(now))) {
					attemptToContactAt(tPtr,_paths[i].p->localSocket(),_paths[i].p->address(),now,sendFullHello);
					_paths[i].p->sent(now);
					sent |= (_paths[i].p->address().ss_family == AF_INET) ? 0x1 : 0x2;
				}
				if (i != j)
					_paths[j] = _paths[i];
				++j;
			}
		} else break;
	}
	return sent;
}

void Peer::clusterRedirect(void *tPtr,const SharedPtr<Path> &originatingPath,const InetAddress &remoteAddress,const int64_t now)
{
	SharedPtr<Path> np(RR->topology->getPath(originatingPath->localSocket(),remoteAddress));
	RR->t->peerRedirected(tPtr,0,*this,np);

	attemptToContactAt(tPtr,originatingPath->localSocket(),remoteAddress,now,true);

	{
		Mutex::Lock _l(_paths_m);

		// New priority is higher than the priority of the originating path (if known)
		long newPriority = 1;
		for(unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
			if (_paths[i].p) {
				if (_paths[i].p == originatingPath) {
					newPriority = _paths[i].priority;
					break;
				}
			} else break;
		}
		newPriority += 2;

		// Erase any paths with lower priority than this one or that are duplicate
		// IPs and add this path.
		unsigned int j = 0;
		for(unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
			if (_paths[i].p) {
				if ((_paths[i].priority >= newPriority)&&(!_paths[i].p->address().ipsEqual2(remoteAddress))) {
					if (i != j)
						_paths[j] = _paths[i];
					++j;
				}
			}
		}
		if (j < ZT_MAX_PEER_NETWORK_PATHS) {
			_paths[j].lr = now;
			_paths[j].p = np;
			_paths[j].priority = newPriority;
			++j;
			while (j < ZT_MAX_PEER_NETWORK_PATHS) {
				_paths[j].lr = 0;
				_paths[j].p.zero();
				_paths[j].priority = 1;
				++j;
			}
		}
	}
}

void Peer::resetWithinScope(void *tPtr,InetAddress::IpScope scope,int inetAddressFamily,int64_t now)
{
	Mutex::Lock _l(_paths_m);
	for(unsigned int i=0;i<ZT_MAX_PEER_NETWORK_PATHS;++i) {
		if (_paths[i].p) {
			if ((_paths[i].p->address().ss_family == inetAddressFamily)&&(_paths[i].p->ipScope() == scope)) {
				attemptToContactAt(tPtr,_paths[i].p->localSocket(),_paths[i].p->address(),now,false);
				_paths[i].p->sent(now);
				_paths[i].lr = 0; // path will not be used unless it speaks again
			}
		} else break;
	}
}

void Peer::recordOutgoingPacket(const SharedPtr<Path> &path, const uint64_t packetId,
	uint16_t payloadLength, const Packet::Verb verb, const int32_t flowId, int64_t now)
{
	if (_localMultipathSupported && _bond) {
		_bond->recordOutgoingPacket(path, packetId, payloadLength, verb, flowId, now);
	}
}

void Peer::recordIncomingInvalidPacket(const SharedPtr<Path>& path)
{
	if (_localMultipathSupported && _bond) {
		_bond->recordIncomingInvalidPacket(path);
	}
}

void Peer::recordIncomingPacket(const SharedPtr<Path> &path, const uint64_t packetId,
	uint16_t payloadLength, const Packet::Verb verb, const int32_t flowId, int64_t now)
{
	if (_localMultipathSupported && _bond) {
		_bond->recordIncomingPacket(path, packetId, payloadLength, verb, flowId, now);
	}
}

} // namespace ZeroTier
