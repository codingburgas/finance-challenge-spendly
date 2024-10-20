#include "mainwindow.h"
#include <QWebChannel>
#include <QJsonObject>
#include <QJsonDocument>
#include "firebase_rest_helper.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), view(new QWebEngineView(this)), firebaseHelper(new FirebaseRestHelper(this))
{
    // Set up the web channel
    QWebChannel* channel = new QWebChannel(this);
    channel->registerObject("backend", this);
    view->page()->setWebChannel(channel);

    // Load the HTML file
    view->load(QUrl("qrc:/html/index.html"));
    view->resize(480, 854); // Mobile-like dimensions
    setCentralWidget(view);

    this->resize(view->size());

    // Connect signals from the Firebase helper to JavaScript functions
    connect(firebaseHelper, &FirebaseRestHelper::authenticationSuccess, this, [=]() {
        runJavaScript("showStatusMessage('Authentication successful!', true);");
    });
    connect(firebaseHelper, &FirebaseRestHelper::authenticationFailed, this, [=](const QString& error) {
        runJavaScript(QString("showStatusMessage('Authentication failed: %1', false);").arg(error));
    });
}

MainWindow::~MainWindow() {
    delete view;
    delete firebaseHelper;
}

void MainWindow::handleAuthenticationRequest(const QString& type, const QString& email, const QString& password) {
    if (type == "signIn") {
        firebaseHelper->signIn(email, password);
    } else if (type == "signUp") {
        firebaseHelper->signUp(email, password);
    }
}

void MainWindow::runJavaScript(const QString& script) {
    view->page()->runJavaScript(script);
}
