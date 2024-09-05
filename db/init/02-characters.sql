CREATE TABLE characters
(
    id      INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,

    name    VARCHAR(30) UNIQUE NOT NULL CHECK (name REGEXP '^[a-zA-Z0-9_]{6,30}$'),
    race    VARCHAR(10)        NOT NULL CHECK (race in ('HUMAN')),

    status  VARCHAR(8)         NOT NULL DEFAULT 'ALIVE' CHECK (status IN ('ALIVE', 'DEAD', 'DELETED')),
    created TIMESTAMP          NOT NULL DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES users (id)
);
CREATE INDEX idx_characters_user_id ON characters (user_id);


CREATE TABLE character_stats
(
    character_id INT PRIMARY KEY,
    level        SMALLINT NOT NULL DEFAULT 1,
    exp          INT NOT NULL DEFAULT 0,

    -- strength
    str          SMALLINT NOT NULL DEFAULT 0,
    -- magic
    mag          SMALLINT NOT NULL DEFAULT 0,
    -- agility
    agi          SMALLINT NOT NULL DEFAULT 0,
    -- constitution
    con          SMALLINT NOT NULL DEFAULT 0,

    -- Optional stats
    faith        SMALLINT NULL,

    FOREIGN KEY (character_id) REFERENCES characters (id)
);


CREATE TABLE character_skills
(
    character_id INT PRIMARY KEY,
    skills       JSON NOT NULL DEFAULT ('{}'),

    FOREIGN KEY (character_id) REFERENCES characters (id)
);


CREATE TABLE character_inventories
(
    character_id INT PRIMARY KEY,
    items        JSON NOT NULL DEFAULT ('{}'),

    FOREIGN KEY (character_id) REFERENCES characters (id)
);


DELIMITER //
CREATE TRIGGER after_character_insert
    AFTER INSERT
    ON characters
    FOR EACH ROW
BEGIN
    INSERT INTO character_stats (character_id)
    VALUES (NEW.id);

    INSERT INTO character_skills (character_id)
    VALUES (NEW.id);

    INSERT INTO character_inventories (character_id)
    VALUES (NEW.id);
END //
DELIMITER ;


DELIMITER //
CREATE TRIGGER after_character_delete
    AFTER DELETE
    ON characters
    FOR EACH ROW
BEGIN
    DELETE
    FROM character_stats
    WHERE character_id = OLD.id;

    DELETE
    FROM character_skills
    WHERE character_id = OLD.id;

    DELETE
    FROM character_inventories
    WHERE character_id = OLD.id;
END //
DELIMITER ;