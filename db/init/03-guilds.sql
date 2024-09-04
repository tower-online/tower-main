CREATE TABLE guilds
(
    id      INT AUTO_INCREMENT PRIMARY KEY,
    name    VARCHAR(30) UNIQUE NOT NULL,
    created TIMESTAMP          NOT NULL DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT check_name CHECK (name REGEXP '^[a-zA-Z0-9_]{6,30}$')
);


CREATE TABLE guild_members
(
    guild_id     INTEGER,
    character_id INTEGER,
    role         VARCHAR(10) NOT NULL CHECK (role IN ('MASTER', 'MANAGER', 'MEMBER')),
    joined       TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (guild_id, character_id),
    FOREIGN KEY (guild_id) REFERENCES guilds (id),
    FOREIGN KEY (character_id) REFERENCES characters (id)
);
