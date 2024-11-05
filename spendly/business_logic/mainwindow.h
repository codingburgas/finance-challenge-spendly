#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include "firebase_rest_helper.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void handleAuthenticationRequest(const QString& type, const QString& email, const QString& password);
    void handleProfileUpdate(const QString& userId, const QString& username, double monthlyIncome);
    void loadHistory(const QString& userId);
    Q_INVOKABLE void exportReceipts(const QJsonArray& receiptsJson);

private:
    void runJavaScript(const QString& script);
    QWebEngineView *view;
    FirebaseRestHelper *firebaseHelper;
};

#endif
