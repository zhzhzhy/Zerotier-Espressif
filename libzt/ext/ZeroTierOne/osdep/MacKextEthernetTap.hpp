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

#ifndef ZT_MacKextEthernetTap_HPP
#define ZT_MacKextEthernetTap_HPP

#include <stdio.h>
#include <stdlib.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../node/Constants.hpp"
#include "../node/MAC.hpp"
#include "../node/InetAddress.hpp"
#include "../node/MulticastGroup.hpp"

#include "Thread.hpp"
#include "EthernetTap.hpp"

namespace ZeroTier {

class MacKextEthernetTap : public EthernetTap
{
public:
	MacKextEthernetTap(
		const char *homePath,
		const MAC &mac,
		unsigned int mtu,
		unsigned int metric,
		uint64_t nwid,
		const char *friendlyName,
		void (*handler)(void *,void *,uint64_t,const MAC &,const MAC &,unsigned int,unsigned int,const void *,unsigned int),
		void *arg);

	virtual ~MacKextEthernetTap();

	virtual void setEnabled(bool en);
	virtual bool enabled() const;
	virtual bool addIp(const InetAddress &ip);
	virtual bool removeIp(const InetAddress &ip);
	virtual std::vector<InetAddress> ips() const;
	virtual void put(const MAC &from,const MAC &to,unsigned int etherType,const void *data,unsigned int len);
	virtual std::string deviceName() const;
	virtual void setFriendlyName(const char *friendlyName);
	virtual void scanMulticastGroups(std::vector<MulticastGroup> &added,std::vector<MulticastGroup> &removed);
	virtual void setMtu(unsigned int mtu);
	virtual void setDns(const char *domain, const std::vector<InetAddress> &servers);


	void threadMain()
		throw();

private:
	void (*_handler)(void *,void *,uint64_t,const MAC &,const MAC &,unsigned int,unsigned int,const void *,unsigned int);
	void *_arg;
	uint64_t _nwid;
	Thread _thread;
	std::string _homePath;
	std::string _dev;
	std::vector<MulticastGroup> _multicastGroups;
	unsigned int _mtu;
	unsigned int _metric;
	int _fd;
	int _shutdownSignalPipe[2];
	volatile bool _enabled;
};

} // namespace ZeroTier

#endif
