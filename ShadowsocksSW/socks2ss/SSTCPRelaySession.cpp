//
//  SSTCPRelaySession.cpp
//  ShadowsocksSW
//
//  Created by Xuhui on 16/8/6.
//  Copyright © 2016年 Xuhui. All rights reserved.
//

#include "SSTCPRelaySession.h"
#include "SSTCPRelayRequest.h"
#include "net/TCPClient.h"
#include "net/Packet.h"
#include "Crypto.h"
#include "SWLogger.h"
#include <cassert>

SSTCPRelaySession::SSTCPRelaySession(const std::shared_ptr<WukongBase::Net::TCPClient>& tcpClient, const std::shared_ptr<Crypto>& crypto)
:   state_(kDisconnected),
    lock_(),
    tcpClient_(tcpClient),
    crypto_(crypto),
    receivedPacketHasIVPrepend_(true)
{
}

SSTCPRelaySession::~SSTCPRelaySession()
{
    assert(tcpSession_->isClosed());
    tcpClient_ = nullptr;
    tcpSession_ = nullptr;
}

const WukongBase::Net::IPAddress& SSTCPRelaySession::getLocalAddress() const
{
    return tcpSession_->getLocalAddress();
}

const WukongBase::Net::IPAddress& SSTCPRelaySession::getPeerAddress() const
{
    return tcpSession_->getPeerAddress();
}

void SSTCPRelaySession::sendRequest(const std::shared_ptr<SSTCPRelayRequest>& request)
{
    std::lock_guard<std::mutex> guard(lock_);
    if(state_ == kDisconnected) {
        state_ = kConnecting;
        tcpClient_->setConnectCallback([this, request](const std::shared_ptr<WukongBase::Net::TCPSession>& tcpSession){
            tcpSession->setReadCompleteCallback([this](std::shared_ptr<WukongBase::Net::Packet>& buffer){
                if(!receivedPacketHasIVPrepend_) {
                    std::shared_ptr<WukongBase::Net::Packet> buf(new WukongBase::Net::Packet(crypto_->decrypt(buffer->data(), (int)buffer->size())));
                    readCompleteCallback_(buf);
                } else {
                    crypto_->setDecryptIV(std::string(buffer->data(), crypto_->getIVLength()));
                    std::shared_ptr<WukongBase::Net::Packet> buf(new WukongBase::Net::Packet(crypto_->decrypt(buffer->data() + crypto_->getIVLength(), (int)buffer->size() - (int)crypto_->getIVLength())));
                    receivedPacketHasIVPrepend_ = false;
                    readCompleteCallback_(buf);
                }
            });
            tcpSession->setWriteCompleteCallback([this, request](const WukongBase::Net::Packet& packet, bool success) {
                lock_.lock();
                if(state_ == kAuthorizing) {
                    if(success) {
                        state_ = kAuthorized;
                    }
                    lock_.unlock();
                    requestCallback_(request, success);
                } else if(state_ == kAuthorized) {
                    lock_.unlock();
                    writeCompleteCallback_(packet, success);
                } else {
                    lock_.unlock();
                }
            });
            tcpSession->setCloseCallback([this, request](bool){
                lock_.lock();
                if(state_ == kAuthorizing) {
                    lock_.unlock();
                    requestCallback_(request, false);
                    lock_.lock();
                }
                state_ = kDisconnected;
                lock_.unlock();
                closeCallback_();
            });
            std::lock_guard<std::mutex> guard(lock_);
            state_ = kAuthorizing;
            SWLOG_DEBUG("ss start auth");
            tcpSession->startRead();
            WukongBase::Net::Packet packet;
            const WukongBase::Net::Packet& p = request->pack();
            packet.append((void*)crypto_->getEncryptIV().c_str(), crypto_->getEncryptIV().size());
            const WukongBase::Net::Packet& buf = crypto_->encrypt(p.data(), (int)p.size());
            packet.append((void*)buf.data(), buf.size());
            tcpSession->send(std::move(packet));
            tcpSession_ = tcpSession;
        });
        tcpClient_->connect();
    }
}

void SSTCPRelaySession::sendPacket(const WukongBase::Net::Packet& packet)
{
    std::lock_guard<std::mutex> guard(lock_);
    if(state_ == kAuthorized) {
        tcpSession_->send(crypto_->encrypt(packet.data(), (int)packet.size()));
    }
}

void SSTCPRelaySession::sendPacket(WukongBase::Net::Packet&& packet)
{
    std::lock_guard<std::mutex> guard(lock_);
    if(state_ == kAuthorized) {
        tcpSession_->send(crypto_->encrypt(packet.data(), (int)packet.size()));
    }
}

void SSTCPRelaySession::close()
{
    tcpSession_->close();
}

bool SSTCPRelaySession::isClosed()
{
    return tcpSession_->isClosed();
}