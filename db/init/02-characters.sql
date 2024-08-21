CREATE TABLE characters
(
    id       INT AUTO_INCREMENT PRIMARY KEY,
    user_id  INT,

    name     VARCHAR(30)    NOT NULL,
    race     ENUM ('HUMAN') NOT NULL,

    is_alive BOOLEAN        NOT NULL DEFAULT TRUE,
    created  TIMESTAMP      NOT NULL DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users (id),

    CONSTRAINT check_name CHECK (name REGEXP '^[a-zA-Z0-9_]{6,30}$')
);
CREATE INDEX idx_characters_user_id ON characters (user_id);

CREATE TABLE character_stats
(
    character_id INT PRIMARY KEY,
    level        INT NOT NULL DEFAULT 1,
    exp          INT NOT NULL DEFAULT 0,

    -- strength
    str          INT NOT NULL DEFAULT 0,
    -- magic
    mag          INT NOT NULL DEFAULT 0,
    -- agility
    agi          INT NOT NULL DEFAULT 0,
    -- constitution
    con          INT NOT NULL DEFAULT 0,

    -- Optional stats
    faith        INT,

    FOREIGN KEY (character_id) REFERENCES characters (id)
);

CREATE TABLE character_skills
(
    character_id INT PRIMARY KEY,
    skills       JSON NOT NULL,

    FOREIGN KEY (character_id) REFERENCES characters (id)
);

CREATE TABLE character_inventories
(
    character_id INT PRIMARY KEY,
    items        JSON NOT NULL,

    FOREIGN KEY (character_id) REFERENCES characters (id)
);