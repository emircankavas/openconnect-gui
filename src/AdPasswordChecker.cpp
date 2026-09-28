#include "AdPasswordChecker.h"

QString AdPasswordChecker::autoBaseDn(const QString& domain)
{
    QString d = domain.trimmed();
    if (d.startsWith("ldap://", Qt::CaseInsensitive)) {
        d = d.mid(7);
    } else if (d.startsWith("ldaps://", Qt::CaseInsensitive)) {
        d = d.mid(8);
    }
    if (d.contains(':')) {
        d = d.section(':', 0, 0);
    }

    QStringList parts = d.split('.', Qt::SkipEmptyParts);
    QStringList dcParts;
    for (const QString& part : parts) {
        dcParts << QString("dc=%1").arg(part);
    }
    return dcParts.join(',');
}

bool AdPasswordChecker::querySingleUser(const QString& domain,
                                        const QString& baseDn,
                                        const QString& bindUser,
                                        const QString& bindPass,
                                        const QString& targetUser,
                                        int& outDays,
                                        QString& outText,
                                        QString& outError)
{
    QString ldapSearchPath = QStandardPaths::findExecutable("ldapsearch");
    if (ldapSearchPath.isEmpty()) {
        ldapSearchPath = "/usr/bin/ldapsearch";
    }

    QString fullBindUser = bindUser.trimmed();
    if (!fullBindUser.contains('@') && !fullBindUser.contains('\\')) {
        QString cleanDomain = domain.trimmed();
        if (cleanDomain.startsWith("ldap://", Qt::CaseInsensitive)) cleanDomain = cleanDomain.mid(7);
        if (cleanDomain.startsWith("ldaps://", Qt::CaseInsensitive)) cleanDomain = cleanDomain.mid(8);
        if (cleanDomain.contains(':')) cleanDomain = cleanDomain.section(':', 0, 0);
        fullBindUser = fullBindUser + "@" + cleanDomain;
    }

    QString host = domain.trimmed();
    if (!host.startsWith("ldap://", Qt::CaseInsensitive) && !host.startsWith("ldaps://", Qt::CaseInsensitive)) {
        host = "ldap://" + host;
    }

    QString sAMAccountName = targetUser.trimmed();
    if (sAMAccountName.contains('@')) {
        sAMAccountName = sAMAccountName.section('@', 0, 0);
    } else if (sAMAccountName.contains('\\')) {
        sAMAccountName = sAMAccountName.section('\\', 1, 1);
    }

    QString bDn = baseDn.trimmed();
    if (bDn.isEmpty()) {
        bDn = autoBaseDn(domain);
    }

    QStringList args;
    args << "-H" << host
         << "-x"
         << "-w" << bindPass
         << "-D" << fullBindUser
         << "-b" << bDn
         << "-o" << "nettimeout=10"
         << "-o" << "ldif-wrap=no"
         << QString("(sAMAccountName=%1)").arg(sAMAccountName)
         << "msDS-UserPasswordExpiryTimeComputed";

    // When VPN first connects, DNS routes take 1-2 seconds to establish.
    // Retry up to 3 times if server contact fails.
    int retries = 3;
    while (retries-- > 0) {
        QProcess process;
        process.start(ldapSearchPath, args);
        if (!process.waitForFinished(10000)) {
            process.kill();
            process.waitForFinished(1000);
            if (retries > 0) {
                QThread::msleep(1500);
                continue;
            }
            outText = QObject::tr("Zaman aşımı");
            outError = "LDAP search timed out";
            outDays = -1;
            return false;
        }

        if (process.exitCode() != 0) {
            QString errStr = QString::fromUtf8(process.readAllStandardError()).trimmed();
            // If DNS/network route is not ready yet, retry after a short delay
            if (retries > 0 && (errStr.contains("Can't contact LDAP server", Qt::CaseInsensitive) ||
                                errStr.contains("ldap_sasl_bind", Qt::CaseInsensitive))) {
                QThread::msleep(1500);
                continue;
            }
            outText = QObject::tr("LDAP Hatası");
            outError = errStr.isEmpty() ? "LDAP search failed" : errStr;
            outDays = -1;
            return false;
        }

        QString stdoutStr = QString::fromUtf8(process.readAllStandardOutput());
        QString computedVal;
        for (const QString& line : stdoutStr.split('\n')) {
            QString trimmed = line.trimmed();
            if (trimmed.startsWith("msDS-UserPasswordExpiryTimeComputed:", Qt::CaseInsensitive)) {
                computedVal = trimmed.section(':', 1).trimmed();
                break;
            }
        }

        if (computedVal.isEmpty()) {
            outText = QObject::tr("Okunamadı");
            outError = "msDS-UserPasswordExpiryTimeComputed bulunamadı";
            outDays = -1;
            return false;
        }

        bool ok = false;
        qint64 computedInt = computedVal.toLongLong(&ok);
        if (!ok) {
            outText = QObject::tr("Geçersiz veri");
            outError = QString("Sayıya çevrilemedi: %1").arg(computedVal);
            outDays = -1;
            return false;
        }

        if (computedInt == 0 || computedInt >= 9223372036854775807LL) {
            outText = QObject::tr("Süresiz");
            outDays = 9999;
            return true;
        }

        qint64 expireUnix = (computedInt / 10000000LL) - 11644473600LL;
        qint64 currentUnix = QDateTime::currentSecsSinceEpoch();
        qint64 diffSeconds = expireUnix - currentUnix;
        qint64 diffDays = diffSeconds / 86400LL;

        if (diffSeconds < 0) {
            outText = QObject::tr("%1 gün önce doldu!").arg(qAbs(diffDays));
            outDays = -1;
        } else if (diffDays == 0) {
            qint64 hours = qMax(1LL, diffSeconds / 3600LL);
            outText = QObject::tr("~%1 saat kaldı").arg(hours);
            outDays = 0;
        } else {
            outText = QObject::tr("%1 gün kaldı").arg(diffDays);
            outDays = static_cast<int>(diffDays);
        }

        return true;
    }

    return false;
}

AdExpiryInfo AdPasswordChecker::check(const QString& domain,
                                      const QString& baseDn,
                                      const QString& username,
                                      const QString& password,
                                      const QString& srvUser)
{
    AdExpiryInfo info;
    if (domain.trimmed().isEmpty()) {
        info.error = "Domain belirtilmedi";
        info.userText = QObject::tr("Domain eksik");
        return info;
    }
    if (username.trimmed().isEmpty()) {
        info.error = "Kullanıcı adı belirtilmedi";
        info.userText = QObject::tr("Kullanıcı eksik");
        return info;
    }
    if (password.isEmpty()) {
        info.error = "Şifre girilmedi";
        info.userText = QObject::tr("Şifre eksik");
        return info;
    }

    QString bDn = baseDn.trimmed().isEmpty() ? autoBaseDn(domain) : baseDn.trimmed();

    QString userErr;
    bool uOk = querySingleUser(domain, bDn, username, password, username, info.userDays, info.userText, userErr);
    if (!uOk) {
        info.error = userErr;
    } else {
        info.success = true;
    }

    if (!srvUser.trimmed().isEmpty()) {
        QString srvErr;
        bool sOk = querySingleUser(domain, bDn, username, password, srvUser.trimmed(), info.srvDays, info.srvText, srvErr);
        if (sOk) {
            info.success = true; // If at least one succeeds, mark success
        } else if (info.error.isEmpty()) {
            info.error = srvErr;
        }
    }

    return info;
}

