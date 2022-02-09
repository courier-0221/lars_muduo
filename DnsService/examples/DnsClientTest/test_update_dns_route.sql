USE lars_dns;

SET @time = UNIX_TIMESTAMP(NOW());

truncate table RouteChange;

UPDATE RouteData SET serverport = 7788 WHERE id = 4;
INSERT INTO RouteChange(modid, cmdid, version) VALUES(1, 2, @time);
UPDATE RouteVersion SET version = @time WHERE id = 1;