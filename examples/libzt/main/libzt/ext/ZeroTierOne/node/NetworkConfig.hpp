/*
 * Copyright (c)2019 ZeroTier, Inc.
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

#ifndef ZT_NETWORKCONFIG_HPP
#define ZT_NETWORKCONFIG_HPP

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <vector>
#include <stdexcept>
#include <algorithm>

#include "../include/ZeroTierOne.h"

#include "Constants.hpp"
#include "Buffer.hpp"
#include "DNS.hpp"
#include "InetAddress.hpp"
#include "MulticastGroup.hpp"
#include "Address.hpp"
#include "CertificateOfMembership.hpp"
#include "CertificateOfOwnership.hpp"
#include "Capability.hpp"
#include "Tag.hpp"
#include "Dictionary.hpp"
#include "Hashtable.hpp"
#include "Identity.hpp"
#include "Utils.hpp"
#include "Trace.hpp"

/**
 * Default time delta for COMs, tags, and capabilities
 */
#define ZT_NETWORKCONFIG_DEFAULT_CREDENTIAL_TIME_DFL_MAX_DELTA ((int64_t)(1000 * 60 * 30))

/**
 * Maximum time delta for COMs, tags, and capabilities
 */
#define ZT_NETWORKCONFIG_DEFAULT_CREDENTIAL_TIME_MAX_MAX_DELTA ((int64_t)(1000 * 60 * 60 * 2))

/**
 * Minimum credential TTL and maxDelta for COM timestamps
 */
#define ZT_NETWORKCONFIG_DEFAULT_CREDENTIAL_TIME_MIN_MAX_DELTA ((int64_t)(1000 * 60 * 5))

/**
 * Flag: enable broadcast
 */
#define ZT_NETWORKCONFIG_FLAG_ENABLE_BROADCAST 0x0000000000000002ULL

/**
 * Flag: enable IPv6 NDP emulation for certain V6 address patterns
 */
#define ZT_NETWORKCONFIG_FLAG_ENABLE_IPV6_NDP_EMULATION 0x0000000000000004ULL

/**
 * Flag: result of unrecognized MATCH entries in a rules table: match if set, no-match if clear
 */
#define ZT_NETWORKCONFIG_FLAG_RULES_RESULT_OF_UNSUPPORTED_MATCH 0x0000000000000008ULL

/**
 * Flag: disable frame compression
 */
#define ZT_NETWORKCONFIG_FLAG_DISABLE_COMPRESSION 0x0000000000000010ULL

/**
 * Device can bridge to other Ethernet networks and gets unknown recipient multicasts
 */
#define ZT_NETWORKCONFIG_SPECIALIST_TYPE_ACTIVE_BRIDGE 0x0000020000000000ULL

/**
 * Anchors are stable devices on this network that can act like roots when none are up
 */
#define ZT_NETWORKCONFIG_SPECIALIST_TYPE_ANCHOR 0x0000040000000000ULL

/**
 * Designated multicast replicators replicate multicast in place of sender-side replication
 */
#define ZT_NETWORKCONFIG_SPECIALIST_TYPE_MULTICAST_REPLICATOR 0x0000080000000000ULL

namespace ZeroTier {

// Dictionary capacity needed for max size network config
#define ZT_NETWORKCONFIG_DICT_CAPACITY (4096 + (sizeof(ZT_VirtualNetworkConfig)) + (sizeof(ZT_VirtualNetworkRule) * ZT_MAX_NETWORK_RULES) + (sizeof(Capability) * ZT_MAX_NETWORK_CAPABILITIES) + (sizeof(Tag) * ZT_MAX_NETWORK_TAGS) + (sizeof(CertificateOfOwnership) * ZT_MAX_CERTIFICATES_OF_OWNERSHIP))

// Dictionary capacity needed for max size network meta-data
#define ZT_NETWORKCONFIG_METADATA_DICT_CAPACITY 1024

// Network config version
#define ZT_NETWORKCONFIG_VERSION 7

// Fields for meta-data sent with network config requests

// Network config version
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_VERSION "v"
// Protocol version (see Packet.hpp)
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_PROTOCOL_VERSION "pv"
// Software vendor
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_NODE_VENDOR "vend"
// Software major version
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_NODE_MAJOR_VERSION "majv"
// Software minor version
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_NODE_MINOR_VERSION "minv"
// Software revision
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_NODE_REVISION "revv"
// Rules engine revision
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_RULES_ENGINE_REV "revr"
// Maximum number of rules per network this node can accept
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_MAX_NETWORK_RULES "mr"
// Maximum number of capabilities this node can accept
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_MAX_NETWORK_CAPABILITIES "mc"
// Maximum number of rules per capability this node can accept
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_MAX_CAPABILITY_RULES "mcr"
// Maximum number of tags this node can accept
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_MAX_NETWORK_TAGS "mt"
// Network join authorization token (if any)
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_AUTH "a"
// Network configuration meta-data flags
#define ZT_NETWORKCONFIG_REQUEST_METADATA_KEY_FLAGS "f"

// These dictionary keys are short so they don't take up much room.
// By convention we use upper case for binary blobs, but it doesn't really matter.

// network config version
#define ZT_NETWORKCONFIG_DICT_KEY_VERSION "v"
// network ID
#define ZT_NETWORKCONFIG_DICT_KEY_NETWORK_ID "nwid"
// integer(hex)
#define ZT_NETWORKCONFIG_DICT_KEY_TIMESTAMP "ts"
// integer(hex)
#define ZT_NETWORKCONFIG_DICT_KEY_REVISION "r"
// address of member
#define ZT_NETWORKCONFIG_DICT_KEY_ISSUED_TO "id"
// remote trace target
#define ZT_NETWORKCONFIG_DICT_KEY_REMOTE_TRACE_TARGET "tt"
// remote trace level
#define ZT_NETWORKCONFIG_DICT_KEY_REMOTE_TRACE_LEVEL "tl"
// flags(hex)
#define ZT_NETWORKCONFIG_DICT_KEY_FLAGS "f"
// integer(hex)
#define ZT_NETWORKCONFIG_DICT_KEY_MULTICAST_LIMIT "ml"
// network type (hex)
#define ZT_NETWORKCONFIG_DICT_KEY_TYPE "t"
// text
#define ZT_NETWORKCONFIG_DICT_KEY_NAME "n"
// network MTU
#define ZT_NETWORKCONFIG_DICT_KEY_MTU "mtu"
// credential time max delta in ms
#define ZT_NETWORKCONFIG_DICT_KEY_CREDENTIAL_TIME_MAX_DELTA "ctmd"
// binary serialized certificate of membership
#define ZT_NETWORKCONFIG_DICT_KEY_COM "C"
// specialists (binary array of uint64_t)
#define ZT_NETWORKCONFIG_DICT_KEY_SPECIALISTS "S"
// routes (binary blob)
#define ZT_NETWORKCONFIG_DICT_KEY_ROUTES "RT"
// static IPs (binary blob)
#define ZT_NETWORKCONFIG_DICT_KEY_STATIC_IPS "I"
// rules (binary blob)
#define ZT_NETWORKCONFIG_DICT_KEY_RULES "R"
// capabilities (binary blobs)
#define ZT_NETWORKCONFIG_DICT_KEY_CAPABILITIES "CAP"
// tags (binary blobs)
#define ZT_NETWORKCONFIG_DICT_KEY_TAGS "TAG"
// tags (binary blobs)
#define ZT_NETWORKCONFIG_DICT_KEY_CERTIFICATES_OF_OWNERSHIP "COO"
// dns (binary blobs)
#define ZT_NETWORKCONFIG_DICT_KEY_DNS "DNS"
// sso enabld
#define ZT_NETWORKCONFIG_DICT_KEY_SSO_ENABLED "ssoe"
// so version
#define ZT_NETWORKCONFIG_DICT_KEY_SSO_VERSION "ssov"
// authentication URL
#define ZT_NETWORKCONFIG_DICT_KEY_AUTHENTICATION_URL "aurl"
// authentication expiry
#define ZT_NETWORKCONFIG_DICT_KEY_AUTHENTICATION_EXPIRY_TIME "aexpt"
// oidc issuer URL
#define ZT_NETWORKCONFIG_DICT_KEY_ISSUER_URL "iurl"
// central endpoint
#define ZT_NETWORKCONFIG_DICT_KEY_CENTRAL_ENDPOINT_URL "ssoce"
// nonce
#define ZT_NETWORKCONFIG_DICT_KEY_NONCE "sson"
// state
#define ZT_NETWORKCONFIG_DICT_KEY_STATE "ssos"
// client ID
#define ZT_NETWORKCONFIG_DICT_KEY_CLIENT_ID "ssocid"

// AuthInfo fields -- used by ncSendError for sso

// AuthInfo Version
#define ZT_AUTHINFO_DICT_KEY_VERSION "aV"
// authenticaiton URL
#define ZT_AUTHINFO_DICT_KEY_AUTHENTICATION_URL "aU"
// issuer URL
#define ZT_AUTHINFO_DICT_KEY_ISSUER_URL "iU"
// Central endpoint URL
#define ZT_AUTHINFO_DICT_KEY_CENTRAL_ENDPOINT_URL "aCU"
// Nonce
#define ZT_AUTHINFO_DICT_KEY_NONCE "aN"
// State
#define ZT_AUTHINFO_DICT_KEY_STATE "aS"
// Client ID
#define ZT_AUTHINFO_DICT_KEY_CLIENT_ID "aCID"

// Legacy fields -- these are obsoleted but are included when older clients query

// boolean (now a flag)
#define ZT_NETWORKCONFIG_DICT_KEY_ENABLE_BROADCAST_OLD "eb"
// IP/bits[,IP/bits,...]
// Note that IPs that end in all zeroes are routes with no assignment in them.
#define ZT_NETWORKCONFIG_DICT_KEY_IPV4_STATIC_OLD "v4s"
// IP/bits[,IP/bits,...]
// Note that IPs that end in all zeroes are routes with no assignment in them.
#define ZT_NETWORKCONFIG_DICT_KEY_IPV6_STATIC_OLD "v6s"
// 0/1
#define ZT_NETWORKCONFIG_DICT_KEY_PRIVATE_OLD "p"
// integer(hex)[,integer(hex),...]
#define ZT_NETWORKCONFIG_DICT_KEY_ALLOWED_ETHERNET_TYPES_OLD "et"
// string-serialized CertificateOfMembership
#define ZT_NETWORKCONFIG_DICT_KEY_CERTIFICATE_OF_MEMBERSHIP_OLD "com"
// node[,node,...]
#define ZT_NETWORKCONFIG_DICT_KEY_ACTIVE_BRIDGES_OLD "ab"
// node;IP/port[,node;IP/port]
#define ZT_NETWORKCONFIG_DICT_KEY_RELAYS_OLD "rl"

// End legacy fields

/**
 * Network configuration received from network controller nodes
 *
 * This is a memcpy()'able structure and is safe (in a crash sense) to modify
 * without locks.
 */
class NetworkConfig
{
public:
	NetworkConfig() :
		networkId(0),
		timestamp(0),
		credentialTimeMaxDelta(0),
		revision(0),
		issuedTo(),
		remoteTraceTarget(),
		flags(0),
		remoteTraceLevel(Trace::LEVEL_NORMAL),
		mtu(0),
		multicastLimit(0),
		specialistCount(0),
		routeCount(0),
		staticIpCount(0),
		ruleCount(0),
		capabilityCount(0),
		tagCount(0),
		certificateOfOwnershipCount(0),
		capabilities(),
		tags(),
		certificatesOfOwnership(),
		type(ZT_NETWORK_TYPE_PRIVATE),
		dnsCount(0),
		ssoEnabled(false),
		authenticationURL(),
		authenticationExpiryTime(0),
		issuerURL(),
		centralAuthURL(),
		ssoNonce(),
		ssoState(),
		ssoClientID()
	{
		name[0] = 0;
		memset(specialists, 0, sizeof(uint64_t)*ZT_MAX_NETWORK_SPECIALISTS);
		memset(routes, 0, sizeof(ZT_VirtualNetworkRoute)*ZT_MAX_NETWORK_ROUTES);
		memset(staticIps, 0, sizeof(InetAddress)*ZT_MAX_ZT_ASSIGNED_ADDRESSES);
		memset(rules, 0, sizeof(ZT_VirtualNetworkRule)*ZT_MAX_NETWORK_RULES);
		memset(&dns, 0, sizeof(ZT_VirtualNetworkDNS));
		memset(authenticationURL, 0, sizeof(authenticationURL));
		memset(issuerURL, 0, sizeof(issuerURL));
		memset(centralAuthURL, 0, sizeof(centralAuthURL));
		memset(ssoNonce, 0, sizeof(ssoNonce));
		memset(ssoState, 0, sizeof(ssoState));
		memset(ssoClientID, 0, sizeof(ssoClientID));
	}

	/**
	 * Write this network config to a dictionary for transport
	 *
	 * @param d Dictionary
	 * @param includeLegacy If true, include legacy fields for old node versions
	 * @return True if dictionary was successfully created, false if e.g. overflow
	 */
	bool toDictionary(Dictionary<ZT_NETWORKCONFIG_DICT_CAPACITY> &d,bool includeLegacy) const;

	/**
	 * Read this network config from a dictionary
	 *
	 * @param d Dictionary (non-const since it might be modified during parse, should not be used after call)
	 * @return True if dictionary was valid and network config successfully initialized
	 */
	bool fromDictionary(const Dictionary<ZT_NETWORKCONFIG_DICT_CAPACITY> &d);

	/**
	 * @return True if broadcast (ff:ff:ff:ff:ff:ff) address should work on this network
	 */
	inline bool enableBroadcast() const { return ((this->flags & ZT_NETWORKCONFIG_FLAG_ENABLE_BROADCAST) != 0); }

	/**
	 * @return True if IPv6 NDP emulation should be allowed for certain "magic" IPv6 address patterns
	 */
	inline bool ndpEmulation() const { return ((this->flags & ZT_NETWORKCONFIG_FLAG_ENABLE_IPV6_NDP_EMULATION) != 0); }

	/**
	 * @return True if frames should not be compressed
	 */
	inline bool disableCompression() const
	{
#ifndef ZT_DISABLE_COMPRESSION
		return ((this->flags & ZT_NETWORKCONFIG_FLAG_DISABLE_COMPRESSION) != 0);
#else
		/* Compression is disabled for libzt builds since it causes non-obvious chaotic
		interference with lwIP's TCP congestion algorithm. Compression is also disabled
		for some NAS builds due to the usage of low-performance processors in certain
		older and budget models. */
		return false;
#endif
	}

	/**
	 * @return Network type is public (no access control)
	 */
	inline bool isPublic() const { return (this->type == ZT_NETWORK_TYPE_PUBLIC); }

	/**
	 * @return Network type is private (certificate access control)
	 */
	inline bool isPrivate() const { return (this->type == ZT_NETWORK_TYPE_PRIVATE); }

	/**
	 * @return ZeroTier addresses of devices on this network designated as active bridges
	 */
	inline std::vector<Address> activeBridges() const
	{
		std::vector<Address> r;
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((specialists[i] & ZT_NETWORKCONFIG_SPECIALIST_TYPE_ACTIVE_BRIDGE) != 0)
				r.push_back(Address(specialists[i]));
		}
		return r;
	}

	inline unsigned int activeBridges(Address ab[ZT_MAX_NETWORK_SPECIALISTS]) const
	{
		unsigned int c = 0;
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((specialists[i] & ZT_NETWORKCONFIG_SPECIALIST_TYPE_ACTIVE_BRIDGE) != 0)
				ab[c++] = specialists[i];
		}
		return c;
	}

	inline bool isActiveBridge(const Address &a) const
	{
		for(unsigned int i=0;i<specialistCount;++i) {
			if (((specialists[i] & ZT_NETWORKCONFIG_SPECIALIST_TYPE_ACTIVE_BRIDGE) != 0)&&(a == specialists[i]))
				return true;
		}
		return false;
	}

	inline std::vector<Address> anchors() const
	{
		std::vector<Address> r;
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((specialists[i] & ZT_NETWORKCONFIG_SPECIALIST_TYPE_ANCHOR) != 0)
				r.push_back(Address(specialists[i]));
		}
		return r;
	}

	inline std::vector<Address> multicastReplicators() const
	{
		std::vector<Address> r;
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((specialists[i] & ZT_NETWORKCONFIG_SPECIALIST_TYPE_MULTICAST_REPLICATOR) != 0)
				r.push_back(Address(specialists[i]));
		}
		return r;
	}

	inline unsigned int multicastReplicators(Address mr[ZT_MAX_NETWORK_SPECIALISTS]) const
	{
		unsigned int c = 0;
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((specialists[i] & ZT_NETWORKCONFIG_SPECIALIST_TYPE_MULTICAST_REPLICATOR) != 0)
				mr[c++] = specialists[i];
		}
		return c;
	}

	inline bool isMulticastReplicator(const Address &a) const
	{
		for(unsigned int i=0;i<specialistCount;++i) {
			if (((specialists[i] & ZT_NETWORKCONFIG_SPECIALIST_TYPE_MULTICAST_REPLICATOR) != 0)&&(a == specialists[i]))
				return true;
		}
		return false;
	}

	inline std::vector<Address> alwaysContactAddresses() const
	{
		std::vector<Address> r;
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((specialists[i] & (ZT_NETWORKCONFIG_SPECIALIST_TYPE_ANCHOR | ZT_NETWORKCONFIG_SPECIALIST_TYPE_MULTICAST_REPLICATOR)) != 0)
				r.push_back(Address(specialists[i]));
		}
		return r;
	}

	inline unsigned int alwaysContactAddresses(Address ac[ZT_MAX_NETWORK_SPECIALISTS]) const
	{
		unsigned int c = 0;
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((specialists[i] & (ZT_NETWORKCONFIG_SPECIALIST_TYPE_ANCHOR | ZT_NETWORKCONFIG_SPECIALIST_TYPE_MULTICAST_REPLICATOR)) != 0)
				ac[c++] = specialists[i];
		}
		return c;
	}

	inline void alwaysContactAddresses(Hashtable< Address,std::vector<InetAddress> > &a) const
	{
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((specialists[i] & (ZT_NETWORKCONFIG_SPECIALIST_TYPE_ANCHOR | ZT_NETWORKCONFIG_SPECIALIST_TYPE_MULTICAST_REPLICATOR)) != 0) {
				a[Address(specialists[i])];
			}
		}
	}

	/**
	 * @param fromPeer Peer attempting to bridge other Ethernet peers onto network
	 * @return True if this network allows bridging
	 */
	inline bool permitsBridging(const Address &fromPeer) const
	{
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((fromPeer == specialists[i])&&((specialists[i] & ZT_NETWORKCONFIG_SPECIALIST_TYPE_ACTIVE_BRIDGE) != 0))
				return true;
		}
		return false;
	}

	inline operator bool() const { return (networkId != 0); }
	inline bool operator==(const NetworkConfig &nc) const { return (memcmp(this,&nc,sizeof(NetworkConfig)) == 0); }
	inline bool operator!=(const NetworkConfig &nc) const { return (!(*this == nc)); }

	/**
	 * Add a specialist or mask flags if already present
	 *
	 * This masks the existing flags if the specialist is already here or adds
	 * it otherwise.
	 *
	 * @param a Address of specialist
	 * @param f Flags (OR of specialist role/type flags)
	 * @return True if successfully masked or added
	 */
	inline bool addSpecialist(const Address &a,const uint64_t f)
	{
		const uint64_t aint = a.toInt();
		for(unsigned int i=0;i<specialistCount;++i) {
			if ((specialists[i] & 0xffffffffffULL) == aint) {
				specialists[i] |= f;
				return true;
			}
		}
		if (specialistCount < ZT_MAX_NETWORK_SPECIALISTS) {
			specialists[specialistCount++] = f | aint;
			return true;
		}
		return false;
	}

	const Capability *capability(const uint32_t id) const
	{
		for(unsigned int i=0;i<capabilityCount;++i) {
			if (capabilities[i].id() == id)
				return &(capabilities[i]);
		}
		return (Capability *)0;
	}

	const Tag *tag(const uint32_t id) const
	{
		for(unsigned int i=0;i<tagCount;++i) {
			if (tags[i].id() == id)
				return &(tags[i]);
		}
		return (Tag *)0;
	}

	/**
	 * Network ID that this configuration applies to
	 */
	uint64_t networkId;

	/**
	 * Controller-side time of config generation/issue
	 */
	int64_t timestamp;

	/**
	 * Max difference between timestamp and tag/capability timestamp
	 */
	int64_t credentialTimeMaxDelta;

	/**
	 * Controller-side revision counter for this configuration
	 */
	uint64_t revision;

	/**
	 * Address of device to which this config is issued
	 */
	Address issuedTo;

	/**
	 * If non-NULL, remote traces related to this network are sent here
	 */
	Address remoteTraceTarget;

	/**
	 * Flags (64-bit)
	 */
	uint64_t flags;

	/**
	 * Remote trace level
	 */
	Trace::Level remoteTraceLevel;

	/**
	 * Network MTU
	 */
	unsigned int mtu;

	/**
	 * Maximum number of recipients per multicast (not including active bridges)
	 */
	unsigned int multicastLimit;

	/**
	 * Number of specialists
	 */
	unsigned int specialistCount;

	/**
	 * Number of routes
	 */
	unsigned int routeCount;

	/**
	 * Number of ZT-managed static IP assignments
	 */
	unsigned int staticIpCount;

	/**
	 * Number of rule table entries
	 */
	unsigned int ruleCount;

	/**
	 * Number of capabilities
	 */
	unsigned int capabilityCount;

	/**
	 * Number of tags
	 */
	unsigned int tagCount;

	/**
	 * Number of certificates of ownership
	 */
	unsigned int certificateOfOwnershipCount;

	/**
	 * Specialist devices
	 *
	 * For each entry the least significant 40 bits are the device's ZeroTier
	 * address and the most significant 24 bits are flags indicating its role.
	 */
	uint64_t specialists[ZT_MAX_NETWORK_SPECIALISTS];

	/**
	 * Statically defined "pushed" routes (including default gateways)
	 */
	ZT_VirtualNetworkRoute routes[ZT_MAX_NETWORK_ROUTES];

	/**
	 * Static IP assignments
	 */
	InetAddress staticIps[ZT_MAX_ZT_ASSIGNED_ADDRESSES];

	/**
	 * Base network rules
	 */
	ZT_VirtualNetworkRule rules[ZT_MAX_NETWORK_RULES];

	/**
	 * Capabilities for this node on this network, in ascending order of capability ID
	 */
	Capability capabilities[ZT_MAX_NETWORK_CAPABILITIES];

	/**
	 * Tags for this node on this network, in ascending order of tag ID
	 */
	Tag tags[ZT_MAX_NETWORK_TAGS];

	/**
	 * Certificates of ownership for this network member
	 */
	CertificateOfOwnership certificatesOfOwnership[ZT_MAX_CERTIFICATES_OF_OWNERSHIP];

	/**
	 * Network type (currently just public or private)
	 */
	ZT_VirtualNetworkType type;

	/**
	 * Network short name or empty string if not defined
	 */
	char name[ZT_MAX_NETWORK_SHORT_NAME_LENGTH + 1];

	/**
	 * Certificate of membership (for private networks)
	 */
	CertificateOfMembership com;

	/**
	 * Number of ZT-pushed DNS configurations
	 */
	unsigned int dnsCount;

	/**
	 * ZT pushed DNS configuration
	 */
	ZT_VirtualNetworkDNS dns;

	/**
	 * SSO enabled flag.
	 */
	bool ssoEnabled;

	/**
	 * SSO verison
	 */
	uint64_t ssoVersion;

	/**
	 * Authentication URL if authentication is required
	 */
	char authenticationURL[2048];

	/**
	 * Time current authentication expires or 0 if external authentication is disabled
	 * 
	 * Not used if authVersion >= 1
	 */
	uint64_t authenticationExpiryTime;

	/**
	 * OIDC issuer URL
	 */
	char issuerURL[2048];

	/**
	 * central base URL.
	 */
	char centralAuthURL[2048];

	/**
	 * sso nonce
	 */
	char ssoNonce[128];

	/**
	 * sso state
	 */
	char ssoState[256];

	/**
	 * oidc client id
	 */
	char ssoClientID[256];
};

} // namespace ZeroTier

#endif
