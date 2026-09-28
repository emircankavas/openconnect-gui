#pragma once

#include <QString>
#include <QDateTime>
#include <QProcess>
#include <QStandardPaths>
#include <QObject>
#include <QThread>

struct AdExpiryInfo {
    bool success{false};
    int userDays{-999};
    QString userText;
    int srvDays{-999};
    QString srvText;
    QString error;
};

class AdPasswordChecker {
public:
    static AdExpiryInfo check(const QString& domain,
                              const QString& baseDn,
                              const QString& username,
                              const QString& password,
                              const QString& srvUser);

    static QString autoBaseDn(const QString& domain);

private:
    static bool querySingleUser(const QString& domain,
                                const QString& baseDn,
                                const QString& bindUser,
                                const QString& bindPass,
                                const QString& targetUser,
                                int& outDays,
                                QString& outText,
                                QString& outError);
};