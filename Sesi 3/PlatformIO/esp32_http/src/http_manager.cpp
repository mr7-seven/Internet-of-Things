#include "http_manager.h"

#include <HTTPClient.h>
#include <WiFi.h>

HttpManager::HttpManager(const char* url, uint32_t timeoutMs)
    : url_(url), timeoutMs_(timeoutMs) {}

int HttpManager::sendPayload(const String& payload) const {
  if (WiFi.status() != WL_CONNECTED) {
    printResult(NOT_CONNECTED);
    return NOT_CONNECTED;
  }

  if (payload.isEmpty()) {
    printResult(EMPTY_PAYLOAD);
    return EMPTY_PAYLOAD;
  }

  HTTPClient http;
  http.setConnectTimeout(timeoutMs_);
  http.setTimeout(timeoutMs_);

  if (!http.begin(url_)) {
    printResult(HTTPC_ERROR_CONNECTION_REFUSED);
    return HTTPC_ERROR_CONNECTION_REFUSED;
  }

  http.addHeader("Content-Type", "application/json");
  const int statusCode = http.POST(payload);
  printResult(statusCode);
  http.end();

  return statusCode;
}

void HttpManager::printResult(int statusCode) const {
  if (statusCode == HTTP_CODE_OK) {
    Serial.println("[HTTP] Success: 200 OK");
    return;
  }

  if (statusCode > 0) {
    Serial.printf("[HTTP] Failed, status code: %d\n", statusCode);
    return;
  }

  if (statusCode == NOT_CONNECTED) {
    Serial.println("[HTTP] Failed: WiFi not connected");
  } else if (statusCode == EMPTY_PAYLOAD) {
    Serial.println("[HTTP] Failed: payload is empty");
  } else {
    Serial.printf("[HTTP] Failed: %s (%d)\n",
                  HTTPClient::errorToString(statusCode).c_str(), statusCode);
  }
}

