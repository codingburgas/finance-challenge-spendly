#include "firebase_rest_helper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

FirebaseRestHelper::FirebaseRestHelper(QObject* parent)
    : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
    connect(networkManager, &QNetworkAccessManager::finished, this, &FirebaseRestHelper::onNetworkReply);
}

void FirebaseRestHelper::signUp(const QString& email, const QString& password) {
    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=%1").arg(apiKey));
    QJsonObject payload = {{"email", email}, {"password", password}, {"returnSecureToken", true}};

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    networkManager->post(request, QJsonDocument(payload).toJson());
}

void FirebaseRestHelper::signIn(const QString& email, const QString& password) {
    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=%1").arg(apiKey));
    QJsonObject payload = {{"email", email}, {"password", password}, {"returnSecureToken", true}};

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    networkManager->post(request, QJsonDocument(payload).toJson());
}

void FirebaseRestHelper::onNetworkReply(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        auto jsonResponse = QJsonDocument::fromJson(reply->readAll()).object();
        if (jsonResponse.contains("idToken")) {
            QString userId = jsonResponse.value("localId").toString();  // Get the user ID
            emit authenticationSuccess(userId);
        } else {
            emit authenticationFailed("Unknown error");
        }
    } else {
        emit authenticationFailed(reply->errorString());
    }
    reply->deleteLater();
}
