-- schemas
-- drop schema if exists `schema_open_jackal`;
create schema if not exists `schema_open_jackal` default character set utf8mb4;

-- users
-- drop user if exists 'u_jackalmfn' @'localhost';
create user if not exists 'u_jackalmfn' @'localhost' IDENTIFIED BY 'p_jackalmfn';

-- tables
-- drop table if exists `schema_open_jackal`.`table_jmfn_node_reports`;
create table if not exists
    `schema_open_jackal`.`table_jmfn_node_reports` (
        `node_id` varchar(40) not null,
        `runners_count` tinyint not null,
        `finished_tasks_count` bigint not null,
        `failed_tasks_count` bigint not null,
        `last_reported` datetime null default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
        `created_at` datetime null default CURRENT_TIMESTAMP,
        primary key (`node_id`)
    );
-- drop table if exists `schema_open_jackal`.`table_jmfn_runners_reports`;
create table if not exists
    `schema_open_jackal`.`table_jmfn_runners_reports` (
        `node_runner_id` varchar(64) not null,
        `runner_running` tinyint not null,
        `tasks_main_queue` json null,
        `tasks_loop_queue` json null,
        `current_mirror_name` varchar(40) null,
        `current_task_url` text null,
        `current_task_dest` text null,
        `use_proxy` tinyint not null default 0,
        `fetchers_count` tinyint not null default 0,
        `runner_stage` tinyint not null default 0,
        `progresses` json null,
        `time_consumed` json null,
        `fetcher_status` json null,
        `request_status` json null,
        `request_result` json null,
        `last_reported` datetime null default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
        `created_at` datetime null default CURRENT_TIMESTAMP,
        primary key (`node_runner_id`)
    );
-- drop table if exists `schema_open_jackal`.`table_jmfn_finished_tasks`;
create table if not exists
    `schema_open_jackal`.`table_jmfn_finished_tasks` (
        `id` bigint not null AUTO_INCREMENT,
        `node_id` varchar(40) not null,
        `mirror_name` varchar(128) null,
        `task_path` text null,
        `created_at` datetime null default CURRENT_TIMESTAMP,
        primary key (`id`)
    );
-- drop table if exists `schema_open_jackal`.`table_jmfn_failed_tasks`;
create table if not exists
    `schema_open_jackal`.`table_jmfn_failed_tasks` (
        `id` bigint not null AUTO_INCREMENT,
        `node_id` varchar(40) not null,
        `mirror_name` varchar(128) null,
        `task_path` text null,
        `created_at` datetime null default CURRENT_TIMESTAMP,
        primary key (`id`)
    );

-- procedures


-- privileges
grant select, insert, update on `schema_open_jackal`.`table_jmfn_node_reports` to 'u_jackalmfn' @'localhost';
grant select, insert, update on `schema_open_jackal`.`table_jmfn_runners_reports` to 'u_jackalmfn' @'localhost';
grant select, insert, update on `schema_open_jackal`.`table_jmfn_finished_tasks` to 'u_jackalmfn'@'localhost';
grant select, insert, update on `schema_open_jackal`.`table_jmfn_failed_tasks` to 'u_jackalmfn'@'localhost';
flush privileges;