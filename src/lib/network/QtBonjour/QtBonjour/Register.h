/*
Copyright (c) 2007, Trenton Schulz

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 3. The name of the author may not be used to endorse or promote products
    derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef BONJOURSERVICEREGISTER_H
#define BONJOURSERVICEREGISTER_H

#include <QtCore/QObject>

#include <QtBonjour/Record.h>
class QSocketNotifier;

// Bonjour flags
#include <stdint.h>
typedef uint32_t DNSServiceFlags;
typedef int32_t DNSServiceErrorType;
typedef struct _DNSServiceRef_t* DNSServiceRef;

class BonjourServiceRegister : public QObject
{
    Q_OBJECT
public:
    BonjourServiceRegister(QObject* parent = 0);
    ~BonjourServiceRegister();

    void registerService(const BonjourRecord& record, quint16 servicePort);

    inline BonjourRecord registeredRecord() const { return finalRecord; }

signals:
    void error(DNSServiceErrorType error);
    void serviceRegistered(const BonjourRecord& record);

private slots:
    void bonjourSocketReadyRead();

private:
    static void bonjourRegisterService(DNSServiceRef sdRef, DNSServiceFlags,
                                       DNSServiceErrorType errorCode,
                                       const char* name, const char* regtype,
                                       const char* domain, void* context);
    DNSServiceRef dnssref;
    QSocketNotifier* bonjourSocket;
    BonjourRecord finalRecord;
};

#endif // BONJOURSERVICEREGISTER_H
