CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(30) UNIQUE NOT NULL CHECK (username ~ '^[a-zA-Z0-9_]{6,30}$'),
    platform VARCHAR(8) NOT NULL CHECK (platform in ('TEST', 'STEAM')),
    status UserStatus NOT NULL CHECK (status in ('ACTIVE', 'INACTIVE', 'BLOCKED')),
    created TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO users (username, platform, status)
VALUES ('dummy_00001', 'TEST', 'ACTIVE');
