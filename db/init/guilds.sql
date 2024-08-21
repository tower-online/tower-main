CREATE TABLE guilds (
    id SERIAL PRIMARY KEY,
    name VARCHAR(30) UNIQUE NOT NULL CHECK (name ~ '^[a-zA-Z0-9_]{4,30}$'),
    created TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE guild_members (
    guild_id INTEGER,
    character_id INTEGER,
    role VARCHAR(10) NOT NULL CHECK (role IN ('MASTER', 'MANAGER', 'MEMBER')),
    joined TIMESTAMP DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (guild_id, character_id),
    FOREIGN KEY (guild_id) REFERENCES guilds(id),
    FOREIGN KEY (character_id) REFERENCES characters(id)
);