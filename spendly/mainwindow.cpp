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
    channel->registerObject("backend", this);  // Register this object to be accessed from JavaScript
    view->page()->setWebChannel(channel);

    // Load the login page (index.html) initially
    view->load(QUrl("qrc:/html/index.html"));

    // Set the dimensions of the window
    view->resize(410, 680);  // Adjust window size
    setCentralWidget(view);

    // Make the window non-resizable
    this->setFixedSize(view->size());

    // Connect signals from Firebase helper to JavaScript functions
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
    // Handle Sign In and Sign Up requests
    if (type == "signIn") {
        firebaseHelper->signIn(email, password);
    } else if (type == "signUp") {
        firebaseHelper->signUp(email, password);
    }
}

void MainWindow::runJavaScript(const QString& script) {
    // Run JavaScript code in the web engine
    view->page()->runJavaScript(script);
}
