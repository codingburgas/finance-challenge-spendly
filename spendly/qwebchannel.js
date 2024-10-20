// Initialize the WebChannel
new QWebChannel(qt.webChannelTransport, function(channel) {
    window.backend = channel.objects.backend;
});

// Function to send authentication requests
function sendAuthenticationRequest(type, email, password) {
    if (window.backend) {
        window.backend.handleAuthenticationRequest(type, email, password);
    } else {
        console.error("WebChannel is not initialized");
    }
}
