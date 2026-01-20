#!/usr/bin/with-contenv bashio

GEMINI_API_KEY=$(bashio::config 'gemini_api_key')
export GEMINI_API_KEY

echo "Starting Gemini Live Proxy..."
python3 /app/proxy.py
