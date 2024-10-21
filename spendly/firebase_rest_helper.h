#ifndef FIREBASE_REST_HELPER_H
#define FIREBASE_REST_HELPER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class FirebaseRestHelper : public QObject {
    Q_OBJECT

public:
    explicit FirebaseRestHelper(QObject* parent = nullptr);
    void signUp(const QString& email, const QString& password);
    void signIn(const QString& email, const QString& password);

signals:
    void authenticationSuccess(const QString& userId);
    void authenticationFailed(const QString& error);

private slots:
    void onNetworkReply(QNetworkReply* reply);

private:
    QNetworkAccessManager* networkManager;
    const QString apiKey = "AIzaSyDUgY1jN4EMveQ7G8LTAIYbov0UVqzbMQk"; // Replace with your actual API key
};

#endif // FIREBASE_REST_HELPER_H
