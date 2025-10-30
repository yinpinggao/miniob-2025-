DROP TABLE IF EXISTS small_test;
CREATE TABLE small_test(id INT, value INT);

INSERT INTO small_test VALUES (1, 100);
INSERT INTO small_test VALUES (2, 50);
INSERT INTO small_test VALUES (3, 200);

SELECT * FROM small_test ORDER BY value ASC;

DROP TABLE small_test;

