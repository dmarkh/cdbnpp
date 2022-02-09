<?php

function cdbnpp_settings() {

	return [

		'settings' => [
			'version' => 1.0,
			'contact_person' => 'Dmitry Arkhipkin, arkhipkin@gmail.com'
		],

		'auth' => [
			'users' => [
				'cdbnpp_ro' => [
					'pass' => 'cdbnpp_ro_pass',
					'access' => 'get'
				],
				'cdbnpp_rw' => [
					'pass' => 'cdbnpp_rw_pass',
					'access' => 'set'
				],
				'cdbnpp_ad' => [
					'pass' => 'cdbnpp_ad_pass',
					'access' => 'admin'
				]
			]
		],

		'db' => 'mysql',

		'mysql' => [

			'read' => [
				'host' => '127.0.0.1',
				'port' => 3306,
				'user' => 'cdbnpp_ro',
				'pass' => 'cdbnpp_ro_pass',
				'dbname' => 'cdbnpp'
			],

			'write' => [
				'host' => '127.0.0.1',
				'port' => 3306,
				'user' => 'cdbnpp_ad',
				'pass' => 'cdbnpp_ad_pass',
				'dbname' => 'cdbnpp'
			],

			'create_table' => [

				'tags' => '
					CREATE TABLE IF NOT EXISTS `cdb_tags` (
				  `id` varchar(36) NOT NULL,
				  `pid` varchar(36) NOT NULL,
				  `name` varchar(128) NOT NULL,
				  `ct` bigint(20) NOT NULL,
			  	`dt` bigint(20) NOT NULL DEFAULT 0,
				  `mode` bigint(20) NOT NULL DEFAULT 0,
				  `tbname` varchar(512) NOT NULL,
				  PRIMARY KEY (`id`,`pid`,`name`,`ct`,`dt`),
				  UNIQUE KEY `cdb_tags_id` (`id`)
					) ENGINE=InnoDB;',

				'schemas' => '
					CREATE TABLE IF NOT EXISTS `cdb_schemas` (
					`id` varchar(36) NOT NULL,
				  `pid` varchar(36) NOT NULL,
				  `ct` bigint(20) NOT NULL,
				  `dt` bigint(20) NOT NULL DEFAULT 0,
				  `data` text NOT NULL,
				  PRIMARY KEY (`id`,`pid`,`ct`,`dt`),
				  UNIQUE KEY `cdb_schemas_id` (`id`),
				  UNIQUE KEY `cdb_schemas_pid` (`pid`)
					) ENGINE=InnoDB;',

				'iov' => '
					CREATE TABLE IF NOT EXISTS `cdb_iov_:tbname:` (
				  `id` varchar(36) NOT NULL,
				  `pid` varchar(36) NOT NULL,
				  `flavor` varchar(128) NOT NULL,
				  `ct` bigint(20) NOT NULL,
				  `dt` bigint(20) NOT NULL DEFAULT 0,
				  `bt` bigint(20) NOT NULL DEFAULT 0,
				  `et` bigint(20) NOT NULL DEFAULT 0,
				  `run` bigint(20) NOT NULL DEFAULT 0,
				  `seq` bigint(20) NOT NULL DEFAULT 0,
				  `fmt` varchar(36) NOT NULL,
				  `uri` varchar(2048) NOT NULL,
					  PRIMARY KEY (`id`,`ct`,`bt`,`dt`,`flavor`),
					  UNIQUE KEY `cdb_iov_calibrations_tpc_struct1_id` (`id`)
					) ENGINE=InnoDB;',

				'data' => '
					CREATE TABLE IF NOT EXISTS `cdb_data_:tbname:` (
					`id` varchar(36) NOT NULL,
					`pid` varchar(36) NOT NULL,
					`ct` bigint(20) NOT NULL,
					`dt` bigint(20) NOT NULL DEFAULT 0,
					`data` text NOT NULL,
					`size` bigint(20) NOT NULL DEFAULT 0,
					PRIMARY KEY (`id`,`pid`,`ct`,`dt`),
					UNIQUE KEY `cdb_data_calibrations_tpc_struct1_id` (`id`)
					) ENGINE=InnoDB'
			],

			'list_tables' => 'SELECT table_name FROM information_schema.tables WHERE table_schema = DATABASE()'

		],

		'pgsql' => [

			'read' => [
				'host' => '127.0.0.1',
				'port' => 5432,
				'user' => 'cdbnpp_ro',
				'pass' => 'cdbnpp_ro_pass',
				'dbname' => 'cdbnpp'
			],

			'write' => [
				'host' => '127.0.0.1',
				'port' => 5432,
				'user' => 'cdbnpp_ad',
				'pass' => 'cdbnpp_ad_pass',
				'dbname' => 'cdbnpp'
			],

			'create_table' => [

				'tags' => '
					CREATE TABLE cdb_tags (
						id   varchar(36),
						pid  varchar(36),
						name varchar(128),
						ct   bigint NOT NULL DEFAULT 0,
						dt   bigint NOT NULL DEFAULT 0,
						mode bigint NOT NULL DEFAULT 0,
						tbname varchar(512),
						CONSTRAINT "cdb_tags_pk" PRIMARY KEY (id, pid, ct, dt),
						CONSTRAINT "cdb_tags_id" UNIQUE (id)
					)
					',

				'schemas' => '
					CREATE TABLE cdb_schemas (
						id   varchar(36),
						pid  varchar(36),
						ct   bigint NOT NULL DEFAULT 0,
						dt   bigint NOT NULL DEFAULT 0,
						data text,
						CONSTRAINT "cdb_schemas_pk" PRIMARY KEY (id, pid, ct, dt),
						CONSTRAINT "cdb_schemas_id" UNIQUE (id),
						CONSTRAINT "cdb_schemas_pid" UNIQUE (pid)
					)
				',

				'iov' => '
					CREATE TABLE cdb_iov_:tbname: (
						id   varchar(36),
						pid  varchar(36),
						flavor varchar(128),
						ct   bigint NOT NULL DEFAULT 0,
						dt   bigint NOT NULL DEFAULT 0,
						bt   bigint NOT NULL DEFAULT 0,
						et   bigint NOT NULL DEFAULT 0,
						run   bigint NOT NULL DEFAULT 0,
						seq   bigint NOT NULL DEFAULT 0,
						fmt  varchar(36),
						uri  varchar(2048),
						CONSTRAINT cdb_iov_:tbname:_pk PRIMARY KEY (id, ct, bt, dt, flavor),
						CONSTRAINT cdb_iov_:tbname:_id UNIQUE (id)
					)
				',

				'data' => '
					CREATE TABLE cdb_data_:tbname: (
						id   varchar(36),
						pid  varchar(36),
						ct   bigint NOT NULL DEFAULT 0,
						dt   bigint NOT NULL DEFAULT 0,
						data text,
						size bigint,
						CONSTRAINT cdb_data_:tbname:_pk PRIMARY KEY (id, pid, ct, dt),
						CONSTRAINT cdb_data_:tbname:_id UNIQUE (id)
					)
				'
			],

			'list_tables' => 'SELECT table_name FROM information_schema.tables WHERE table_schema = \'public\''

		]

	];
}
