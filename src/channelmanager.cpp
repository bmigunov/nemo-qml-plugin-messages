/* Copyright (C) 2012 John Brooks <john.brooks@dereferenced.net>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "channelmanager.h"
#include "conversationchannel.h"
#include <QPointer>

#include <CommHistory/recipient.h>

#include <TelepathyQt/ChannelClassSpec>
#include <TelepathyQt/ReceivedMessage>
#include <TelepathyQt/TextChannel>
#include <TelepathyQt/ChannelRequest>
#include <TelepathyQt/Account>
#include <TelepathyQt/ClientRegistrar>

using namespace Tp;

class TpClientHandler : public Tp::AbstractClientHandler
{
public:
    QPointer<ChannelManager> manager;

    TpClientHandler(ChannelManager *manager)
        : AbstractClientHandler(ChannelClassSpec::textChat())
        , manager(manager)
    {
    }

    virtual bool bypassApproval() const;
    virtual void handleChannels(const Tp::MethodInvocationContextPtr<> &context, const Tp::AccountPtr &account,
                                const Tp::ConnectionPtr &connection, const QList<Tp::ChannelPtr> &channels,
                                const QList<Tp::ChannelRequestPtr> &requestsSatisfied, const QDateTime &userActionTime,
                                const HandlerInfo &handlerInfo);
};

ChannelManager::ChannelManager(QObject *parent)
    : QObject(parent)
{
}

ChannelManager::~ChannelManager()
{
}

QString ChannelManager::handlerName() const
{
    return m_handlerName;
}

void ChannelManager::setHandlerName(const QString &name)
{
    if (name.isEmpty() || !m_handlerName.isEmpty())
        return;

    m_handlerName = name;

    const QDBusConnection &dbus = QDBusConnection::sessionBus();
    if (registrar.isNull())
        registrar = ClientRegistrar::create(dbus);
    handler = AbstractClientPtr(new TpClientHandler(this));
    registrar->registerClient(handler, m_handlerName);

    emit handlerNameChanged();
}

ConversationChannel *ChannelManager::getConversation(const QString &localUid, const QString &remoteUid)
{
    const CommHistory::Recipient recipient(localUid, remoteUid);
    foreach (ConversationChannel *channel, channels) {
        const CommHistory::Recipient channelRecipient(channel->localUid(), channel->remoteUid());
        // Recipient point of view localUid comparison doesn't make sense.
        // However, when it comes to ConversationChannels the localUid must match.
        if (channel->localUid() == localUid && channelRecipient.matches(recipient)) {
            return channel;
        }
    }

    ConversationChannel *channel = new ConversationChannel(localUid, remoteUid, this);
    connect(channel, SIGNAL(destroyed(QObject*)), SLOT(channelDestroyed(QObject*)));
    channels.append(channel);

    return channel;
}

void ChannelManager::channelDestroyed(QObject *obj)
{
    if (ConversationChannel *channel = static_cast<ConversationChannel*>(obj)) {
        channel->channelDestroyed();
        channels.removeOne(channel);
    }
}

bool TpClientHandler::bypassApproval() const
{
    return true;
}

void TpClientHandler::handleChannels(const MethodInvocationContextPtr<> &context, const AccountPtr &account,
                                     const ConnectionPtr &connection, const QList<ChannelPtr> &channels,
                                     const QList<ChannelRequestPtr> &requestsSatisfied, const QDateTime &userActionTime,
                                     const HandlerInfo &handlerInfo)
{
    Q_UNUSED(connection);
    Q_UNUSED(requestsSatisfied);
    Q_UNUSED(userActionTime);
    Q_UNUSED(handlerInfo);

    if (manager.isNull()) {
        context->setFinished();
        return;
    }

    foreach (const ChannelPtr &channel, channels) {
        QVariantMap properties = channel->immutableProperties();
        QString targetId = properties.value(TP_QT_IFACE_CHANNEL + QLatin1String(".TargetID")).toString();

        if (targetId.isEmpty()) {
            qWarning() << "handleChannels cannot get TargetID for channel";
            continue;
        }

        ConversationChannel *c = manager->getConversation(account->objectPath(), targetId);
        if (!c) {
            qWarning() << "handleChannels cannot create ConversationChannel";
            continue;
        }

        c->addChannel(channel);
    }

    context->setFinished();
}

