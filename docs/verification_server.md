## 検証環境構築

### 概要

このドキュメントでは、Dockerを用いてHTTPサーバー、HTTPSサーバー、フォワードプロキシサーバーの3つのサーバーからなる検証環境を構築します。

### 1. 環境準備

* Docker: Dockerがインストールされ、動作していることを確認してください。
* docker-compose: docker-composeがインストールされ、動作していることを確認してください。

### 2. Dockerイメージの準備

以下の3つのDockerfileを作成し、それぞれ`proxy`, `web`, `https-web`ディレクトリに配置します。

#### 2.1 フォワードプロキシサーバー (Squid)

`proxy/squid.conf`

```
http_port 3128

# アクセス制御リスト - 必要に応じて編集
# acl allowed_sites dstdomain example.com 
# http_access allow allowed_sites

http_access allow all
```

#### 2.2 HTTPサーバー (Nginx)

`web/default.conf`

```nginx
server {
  listen 80;
  location / {
    add_header Content-Type text/plain;
    return 200 'Hello from HTTP server!\r\n';
  }

  # /certにアクセスすると証明書を表示
  location /cert {
    add_header Content-Type text/plain;
    root /certs;
    try_files /nginx.crt =404;
  }
}
```

#### 2.3 HTTPSサーバー (Nginx)

`https-web/Dockerfile`

```dockerfile
FROM nginx:latest

# /etc/nginx/ssl ディレクトリを作成
RUN mkdir -p /etc/nginx/ssl

# 自己署名証明書と秘密鍵を生成
RUN openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -subj "/C=JP/ST=Tokyo/L=Tokyo/O=Example/CN=192.168.0.160" \
    -keyout /etc/nginx/ssl/nginx.key \
    -out /etc/nginx/ssl/nginx.crt

# /certs ディレクトリを作成
RUN mkdir -p /certs 

# 証明書を共有ボリュームにコピー
RUN cp /etc/nginx/ssl/nginx.crt /certs/nginx.crt

COPY default.conf /etc/nginx/conf.d/default.conf
```

`https-web/default.conf`

```nginx
server {
  listen 443 ssl;
  server_name 192.168.0.160;

  ssl_certificate /etc/nginx/ssl/nginx.crt;
  ssl_certificate_key /etc/nginx/ssl/nginx.key;

  location / {
    add_header Content-Type text/plain;
    return 200 'Hello from HTTPS server!';
  }

  # /certにアクセスすると証明書を表示
  location /cert {
    add_header Content-Type text/plain;
    root /etc/nginx/ssl;
    try_files /nginx.crt =404;
  }
}
```

### 3. Docker Composeファイルの作成

3つのサーバーをまとめて起動するために、`docker-compose.yml` ファイルを作成します。

```yaml
version: "3.7"
services:
  proxy:
    image: ubuntu/squid
    ports:
      - "3128:3128"
  web:
    image: nginx:latest
    ports:
      - "80:80"
    volumes:
      - ./web/default.conf:/etc/nginx/conf.d/default.conf
      - certs:/certs 
    depends_on:
      - https-web
    volumes_from:
      - https-web
  https-web:
    build: ./https-web
    ports:
      - "443:443"
    volumes:
      - ./https-web/default.conf:/etc/nginx/conf.d/default.conf
      - certs:/certs 
  
volumes:
  certs:

```

### 4. Dockerコンテナの起動

`docker-compose.yml` ファイルがあるディレクトリで以下のコマンドを実行し、コンテナを起動します。

```bash
docker-compose up -d --build
```

### 5. 自己署名証明書の確認

"http://HTTPサーバー/cert/"を開き、証明書を確認する。

### 6. 後片付け

検証が終了したら、以下のコマンドでコンテナを停止・削除します。

```bash
docker-compose down
```