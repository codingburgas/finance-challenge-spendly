#include "firebase_rest_helper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QNetworkAccessManager>

FirebaseRestHelper::FirebaseRestHelper(QObject* parent) : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
    connect(networkManager, &QNetworkAccessManager::finished, this, &FirebaseRestHelper::onNetworkReply);
}

void FirebaseRestHelper::signUp(const QString& email, const QString& password) {
    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=%1").arg(apiKey));
    QJsonObject payload;
    payload["email"] = email;
    payload["password"] = password;
    payload["returnSecureToken"] = true;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    networkManager->post(request, QJsonDocument(payload).toJson());
}

void FirebaseRestHelper::signIn(const QString& email, const QString& password) {
    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=%1").arg(apiKey));
    QJsonObject payload;
    payload["email"] = email;
    payload["password"] = password;
    payload["returnSecureToken"] = true;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    networkManager->post(request, QJsonDocument(payload).toJson());
}

void FirebaseRestHelper::onNetworkReply(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        // Success response handling
        QJsonDocument responseDoc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject responseObject = responseDoc.object();
        if (responseObject.contains("idToken")) {
            emit authenticationSuccess();
        } else {
            emit authenticationFailed("Unknown error");
        }
    } else {
        // Error response handling
        emit authenticationFailed(reply->errorString());
    }
    reply->deleteLater();
}
