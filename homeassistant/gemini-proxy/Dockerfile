ARG BUILD_FROM
FROM $BUILD_FROM

# Install requirements for the add-on
RUN \
  apk add --no-cache \
    python3 \
    py3-pip \
    py3-numpy \
    py3-scipy \
    gcc \
    g++ \
    make \
    musl-dev \
    linux-headers

# Install Python dependencies directly to avoid missing file issues
RUN pip3 install \
    google-genai \
    numpy \
    scipy \
    onnxruntime \
    --break-system-packages

WORKDIR /app
COPY . /app
RUN chmod a+x /app/run.sh

CMD [ "/app/run.sh" ]
