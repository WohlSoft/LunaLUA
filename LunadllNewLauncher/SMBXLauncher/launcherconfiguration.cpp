#include "launcherconfiguration.h"
#include <QJsonObject>

#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QApplication>

QJsonDocument LauncherConfiguration::generateDefault()
{
    return QJsonDocument(
        QJsonObject
        {
            {"game", QJsonObject
                {
                    {"update-check-website", "."},
                    {"version-1", 0},
                    {"version-2", 0},
                    {"version-3", 0},
                    {"version-4", 0}
                }
            }
        }
    );

}

LauncherConfiguration::LauncherConfiguration(const QJsonDocument &settingsToParse)
{
    QJsonObject mainObject = settingsToParse.object();
    QJsonObject gameValue = mainObject.value("game").toObject();
    updateCheckWebsite = gameValue.value("update-check-website").toString(".");
    version1 = gameValue.value("version-1").toInt(0);
    version2 = gameValue.value("version-2").toInt(0);
    version3 = gameValue.value("version-3").toInt(0);
    version4 = gameValue.value("version-4").toInt(0);
}

#include <iostream>

bool LauncherConfiguration::checkForUpdate(QJsonDocument *result, UpdateCheckerErrCodes& errCode, QString& errDescription)
{
    errDescription = "";
    if(updateCheckWebsite.isEmpty() || updateCheckWebsite == "."){
        errCode = UERR_NO_URL;
        return false;
    }

    QUrl urlToUpdateChecker(updateCheckWebsite);
    if(!urlToUpdateChecker.isValid()){
        errCode = UERR_INVALID_URL;
        errDescription = urlToUpdateChecker.errorString();
        return false;
    }

    QNetworkReply * rpl = nullptr;
    bool replyFinished = false;

    QNetworkAccessManager downloader;
    downloader.setNetworkAccessible(QNetworkAccessManager::Accessible);

    QObject::connect(&downloader, &QNetworkAccessManager::finished, [&rpl, &replyFinished](QNetworkReply *reply){
        replyFinished = true;
        rpl = reply;
    });

    QNetworkRequest jsonRequest(urlToUpdateChecker);
    downloader.get(jsonRequest);

    while(!replyFinished)
        qApp->processEvents();



    QByteArray data = rpl->readAll();
    if(data.isEmpty()){
        errCode = UERR_CONNECTION_FAILED;
        errDescription = rpl->errorString();
        return false;
    }

    QJsonParseError err;
    *result = QJsonDocument::fromJson(data, &err);

    if(err.error != QJsonParseError::NoError){
        errCode = UERR_INVALID_JSON;
        errDescription = err.errorString();
        return false;
    }

    rpl->deleteLater();

    errCode = UERR_NO_ERR;
    return true;
}

bool LauncherConfiguration::hasHigherVersion(int ver1, int ver2, int ver3, int ver4)
{
    if(ver1 > version1) return true;
    if(ver1 < version1) return false;
    if(ver2 > version2) return true;
    if(ver2 < version2) return false;
    if(ver3 > version3) return true;
    if(ver3 < version3) return false;
    if(ver4 > version4) return true;
    if(ver4 < version4) return false;
    return false;
}