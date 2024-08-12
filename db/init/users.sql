CREATE TYPE UserStatus AS ENUM ('ACTIVE', 'INACTIVE', 'BLOCKED');

CREATE TYPE UserPlatform AS ENUM ('TEST', 'STEAM');

CREATE TABLE users (
    id SERIAL NOT NULL PRIMARY KEY,
    username VARCHAR(30) UNIQUE NOT NULL CHECK (username ~ '^[a-zA-Z0-9_]{6,30}$'),
    platform UserPlatform NOT NULL,
    status UserStatus NOT NULL,
    created TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO users (username, platform, status)
VALUES ('leesin', 'TEST', 'ACTIVE');
