#include "firebase_rest_helper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QDebug>

FirebaseRestHelper::FirebaseRestHelper(QObject* parent)
    : QObject(parent), networkManager(new QNetworkAccessManager(this)) {
    connect(networkManager, &QNetworkAccessManager::finished, this, &FirebaseRestHelper::onNetworkReply);
}

void FirebaseRestHelper::signUp(const QString& email, const QString& password) {
    if (apiKey.isEmpty()) {
        emit authenticationFailed("API Key not set");
        return;
    }
    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=%1").arg(apiKey));
    QJsonObject payload = {{"email", email}, {"password", password}, {"returnSecureToken", true}};

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    networkManager->post(request, QJsonDocument(payload).toJson());
}

void FirebaseRestHelper::signIn(const QString& email, const QString& password) {
    if (apiKey.isEmpty()) {
        emit authenticationFailed("API Key not set");
        return;
    }
    QUrl url(QString("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=%1").arg(apiKey));
    QJsonObject payload = {{"email", email}, {"password", password}, {"returnSecureToken", true}};

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    networkManager->post(request, QJsonDocument(payload).toJson());
}

void FirebaseRestHelper::updateUserProfile(const QString& userId, const QString& username, int monthlyIncome) {
    if (userId.isEmpty()) {
        qWarning() << "User ID is empty";
        return;
    }
    QUrl url(QString("https://firestore.googleapis.com/v1/projects/spendly-3412a/databases/(default)/documents/users/%1").arg(userId));
    QJsonObject data;
    data["fields"] = QJsonObject{
        {"username", QJsonObject{{"stringValue", username}}},
        {"monthlyIncome", QJsonObject{{"integerValue", QString::number(monthlyIncome)}}}
    };

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + idToken.toUtf8()); // Add authentication token
    networkManager->sendCustomRequest(request, "PATCH", QJsonDocument(data).toJson());
}

void FirebaseRestHelper::fetchUserProfile(const QString& userId) {
    if (userId.isEmpty() || idToken.isEmpty()) {
        qWarning() << "User ID or ID Token is empty";
        return;
    }
    QUrl url(QString("https://firestore.googleapis.com/v1/projects/spendly-3412a/databases/(default)/documents/users/%1").arg(userId));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + idToken.toUtf8());

    networkManager->get(request);
}

void FirebaseRestHelper::onNetworkReply(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        auto jsonResponse = QJsonDocument::fromJson(reply->readAll()).object();
        qDebug() << "JSON Response:" << jsonResponse;

        // Handle authentication response
        if (jsonResponse.contains("idToken")) {
            QString userId = jsonResponse.value("localId").toString();
            idToken = jsonResponse.value("idToken").toString();
            emit authenticationSuccess(userId); // Emit only `userId`, matching the updated signal declaration
        }
        // Handle profile response
        else if (jsonResponse.contains("fields") && jsonResponse["fields"].isObject()) {
            QJsonObject fields = jsonResponse["fields"].toObject();
            QString username;
            int monthlyIncome = 0;

            if (fields.contains("username") && fields["username"].isObject()) {
                username = fields["username"].toObject().value("stringValue").toString();
            }
            if (fields.contains("monthlyIncome") && fields["monthlyIncome"].isObject()) {
                monthlyIncome = fields["monthlyIncome"].toObject().value("integerValue").toString().toInt();
            }
            emit userProfileFetched(username, monthlyIncome);
        }
    } else {
        qWarning() << "Network Error:" << reply->errorString();
        emit authenticationFailed(reply->errorString());
    }
    reply->deleteLater();
}
