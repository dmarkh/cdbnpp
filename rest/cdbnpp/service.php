<?php

namespace CDBNPP;

use PDO;
use PDOException;

class Service {

	private $dbh = null;
	private $mode = 'read';

  public function __construct() {
  }

	public function __destruct() {
		$this->disconnect();
	}

  public function &Instance () {
    static $instance;
    if (!isset($instance)) { $c = __CLASS__; $instance = new $c; }
    return $instance;
  }

	private function is_ready() {
		return $this->pdo;
	}

	private function connect_read() {
		$this->dbh = null;
		$this->mode = 'read';
		$db = Config::Instance()->get('db');
		$dbcfg = Config::Instance()->get( $db, 'read' );
		return $this->connect( $db, $dbcfg );
	}

	private function connect_write() {
		$this->dbh = null;
		$this->mode = 'write';
		$db = Config::Instance()->get('db');
		$dbcfg = Config::Instance()->get( $db, $this->mode );
		return $this->connect( $db, $dbcfg );
	}

	private function connect($driver, $cfg) {
		$host = $cfg['host'];
		$port = $cfg['port'];
		$dbname = $cfg['dbname'];
		$user = $cfg['user'];
		$pass = $cfg['pass'];
		$charset = 'utf8mb4';
		$dsn = $driver.':host='.$host.';dbname='.$dbname.';port='.$port;
		$options = [
    	PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
			PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
    	PDO::ATTR_EMULATE_PREPARES   => false,
		];
		try {
     $this->dbh = new PDO( $dsn, $user, $pass, $options );
		} catch (\PDOException $e) {
    	return [ 'error' => 'DB ERROR: ' . $e->getMessage(), 'code' => (int)$e->getCode() ];
		}
		return true;
	}

	private function disconnect() {
		$this->dbh = null;
	}

	// API FUNCTIONS TO RETRIEVE DATA FROM THE DB

	public function initialize() {

		$c = $this->connect_write();
		if ( is_array($c) && !empty($c['error']) ) {
			return $c;
		}
		// TODO:
		return [ 'initialize' => 1 ];
	}

	// -------------------------------------------------------------------------------------------------------------

	public function download() {
		$c = $this->connect_read();
		if ( is_array($c) && !empty($c['error']) ) {
			return $c;
		}

		$tbname = !empty($_GET['tbname']) ? $_GET['tbname'] : '';
		$id = !empty($_GET['id']) ? $_GET['id'] : '';
		sanitize_alnumscore( $tbname );
		sanitize_alnumdash( $id );

		if ( empty($tbname) || empty($id) ) {
			return [ 'error' => 'empty name or id' ];
		}

		try {
			$stmt = $this->dbh->prepare('SELECT * FROM cdb_data_'.$tbname.' WHERE id = :id');
			$stmt->execute([ 'id' => $id ]);
		} catch( PDOException $e ) {
			return [ 'error' => $e->getMessage() ];
		}

		$res = $stmt->fetch(PDO::FETCH_ASSOC);
		if ( empty($res) || empty($res['data']) ) {
			return [ 'error' => 'no data found' ];
		}

		return $res['data'];
	}

	public function tables() {

		$c = $this->connect_write();
		if ( is_array($c) && !empty($c['error']) ) {
			error_log( print_r($c, true), 0 );
			return $c;
		}

		$op = !empty($_POST['op']) ? sanitize_alnum($_POST['op']) : '';

		if ( empty($op) ) {
			return [ 'error' => 'no operation provided' ];
		}

		if ( $op == 'list' ) {
			try {
				$db = Config::Instance()->get('db');
				$list_tables = Config::Instance()->get( $db, 'list_tables' );
				$tables = $this->dbh->query( $list_tables )->fetchAll(PDO::FETCH_ASSOC);
				if ( !empty($tables) ) {
					$tbl = [];
					foreach( $tables as $k => $v ) {
						$tbl[] = $v['table_name'];
					}
					return [ 'tables' => $tbl ];
				} else {
					return [ 'tables' => [] ];
				}
			} catch( PDOException $e ) {
				return [ 'error' => $e->getMessage() ];
			}

		} else if ( $op == 'create' ) {

				$db = Config::Instance()->get('db');
				$recipes = Config::Instance()->get( $db, 'create_table' );

				try {
					$this->dbh->query( $recipes['tags'] );
					$this->dbh->query( $recipes['schemas'] );
				} catch( PDOException $e ) {
					return [ 'error' => $e->getMessage() ];
				}

				return [ 'success' => true ];

		} else if ( $op == 'drop' ) {

				try {

					$db = Config::Instance()->get('db');
					$list_tables = Config::Instance()->get( $db, 'list_tables' );
					$tables = $this->dbh->query( $list_tables )->fetchAll(PDO::FETCH_ASSOC);
					$tbl = [];
					if ( !empty($tables) ) {
						foreach( $tables as $k => $v ) {
							$tbl[] = $v['table_name'];
						}
						foreach( $tbl as $k => $v ) {
							$this->dbh->query( 'drop table '.$v );
						}
					}
					return [ 'success' => true ];

				} catch( PDOException $e ) {
					return [ 'error' => $e->getMessage() ];
				}

		} else {
			return [ 'error' => 'unknown operation' ];
		}

		return [ 'success' => 1 ];
	}

	public function tags() {

		$c = $this->connect_read();
		if ( is_array($c) && !empty($c['error']) ) {
			return $c;
		}

		try {
			$tags = $this->dbh->query('SELECT t.id, t.name, t.pid, t.tbname, t.ct, t.dt, t.mode, COALESCE(s.id,\'\') as schema_id FROM cdb_tags t LEFT JOIN cdb_schemas s ON t.id = s.pid')->fetchAll(PDO::FETCH_ASSOC);
		} catch( PDOException $e ) {
			return [ 'error' => $e->getMessage() ];
		}

		return [ 'tags' => !empty($tags) ? $tags : [] ];
	}

	// -------------------------------------------------------------------------------------------------------------
	public function tag() {

		$c = $this->connect_write();
		if ( is_array($c) && !empty($c['error']) ) {
			return $c;
		}

		$id = !empty($_POST['id'] ) ? sanitize_alnumdash( $_POST['id'] ) : '';
		$pid = !empty($_POST['pid'] ) ? sanitize_alnumdash( $_POST['pid'] ) : '';
		$name = !empty($_POST['name'] ) ? sanitize_alnum( $_POST['name'] ) : '';
		$tbname = !empty($_POST['tbname'] ) ? sanitize_alnumscore( $_POST['tbname'] ) : '';
		$ct = !empty( $_POST['ct'] ) ? intval( $_POST['ct'] ) : 0;
		$dt = !empty( $_POST['dt'] ) ? intval( $_POST['dt'] ) : 0;
		$mode = !empty( $_POST['mode'] ) ? intval( $_POST['mode'] ) : 0;
		$op = !empty($_POST['op']) ? sanitize_alnum($_POST['op']) : '';

		if ( $op == 'deactivate' && !empty($id) ) {
			// deactivate tag
			try {
				$stmt = $this->dbh->prepare('UPDATE cdb_tags SET dt = :dt WHERE id = :id');
				$stmt->execute([ 'dt' => $dt, 'id' => $id ]);
			} catch( PDOException $e ) {
				return [ 'error' => $e->getMessage() ];
			}
			return [ 'uuid' => $id ];

		} else if ( $op == 'create' && !empty($id) && !empty($name) ) {

			// create new tag
			try {
				$stmt = $this->dbh->prepare('INSERT INTO cdb_tags ( id, pid, name, tbname, ct, dt, mode ) VALUES ( :id, :pid, :name, :tbname, :ct, :dt, :mode )');
				$stmt->execute([ 'id' => $id, 'pid' => $pid, 'name' => $name, 'tbname' => $tbname, 'ct' => $ct, 'dt' => $dt, 'mode' => $mode ]);

				// create cdb_data_, cdb_iov_ tables
				if ( !empty($tbname) ) {
    	    $db = Config::Instance()->get('db');
      	  $recipes = Config::Instance()->get( $db, 'create_table' );
					$create_iov_table_query = str_replace( ':tbname:', $tbname, $recipes['iov'] );
					$create_data_table_query = str_replace( ':tbname:', $tbname, $recipes['data'] );
					$this->dbh->query( $create_iov_table_query );
					$this->dbh->query( $create_data_table_query );
				}

			} catch( PDOException $e ) {
				return [ 'error' => $e->getMessage() ];
			}
			return [ 'uuid' => $id ];
		}

		error_log('unknown operation', 0);
		return [ 'error' => 'unknown tag operation: neither create, nor deactivate' ];
	}

	// -------------------------------------------------------------------------------------------------------------
	public function schema_get() {

		$c = $this->connect_read();
		if ( is_array($c) && !empty($c['error']) ) {
			return $c;
		}

		$tag_id = $_GET['id'] ?? '';
		sanitize_alnumdash($tag_id);

		if ( empty($tag_id) ) {
			return [ 'error' => 'tag id is not provided' ];
		}

		try {
			$stmt = $this->dbh->prepare('SELECT * FROM cdb_schemas WHERE pid = :pid');
			$stmt->execute([ 'pid' => $tag_id ]);
		} catch( PDOException $e ) {
			return [ 'error' => $e->getMessage() ];
		}

		$schema = $stmt->fetch(PDO::FETCH_ASSOC);
		if ( empty($schema) || empty($schema['data']) ) {
			return [ 'error' => 'no schema for pid: ' . $tag_id ];
		}

		return [ 'tag_id' => $tag_id, 'schema' => $schema['data'] ];
	}

	public function schema_post() {

		$c = $this->connect_write();
		if ( is_array($c) && !empty($c['error']) ) {
			return $c;
		}

		$data['op'] = !empty( $_POST['op'] ) ? sanitize_alnum($_POST['op']) : '';
		$data['id'] = !empty( $_POST['id'] ) ? sanitize_alnumdash($_POST['id']) : uuidv4();
		$data['pid'] = !empty( $_POST['pid'] ) ? sanitize_alnumdash( $_POST['pid'] ) : '';
		$data['schema'] = !empty($_POST['schema']) ? $_POST['schema'] : '';
		$data['ct'] = !empty($_POST['ct']) ? intval($_POST['ct']) : 0;
		$data['dt'] = !empty($_POST['dt']) ? intval($_POST['dt']) : 0;

		if ( $data['op'] == 'create' ) {

			if ( empty($data['pid']) || empty($data['schema']) ) {
				return [ 'error' => 'malformed request, no pid or no schema provided' ];
			}

			try {
				$stmt = $this->dbh->prepare('INSERT INTO cdb_schemas ( id, pid, ct, dt, data ) VALUES ( :id, :pid, :ct, 0, :data )');
				$stmt->execute([ 'id' => $data['id'], 'pid' => $data['pid'], 'ct' => $data['ct'], 'data' => $data['schema'] ]);
			} catch ( PDOException $e ) {
				return [ 'error' => $e->getMessage() ];
			}

			return [ 'id' => $data['id'] ];
		} else if ( $data['op'] == 'drop' ) {

			if ( empty($data['pid']) ) {
				return [ 'error' => 'malformed request, no pid provided' ];
			}

			try {
				$stmt = $this->dbh->prepare('DELETE FROM cdb_schemas WHERE pid = :pid');
				$stmt->execute([ 'pid' => $data['pid'] ]);
			} catch ( PDOException $e ) {
				return [ 'error' => $e->getMessage() ];
			}

			return [ 'pid' => $data['pid'] ];
		}

		return [ 'error' => 'unknown operation on schema' ];
	}

	// -------------------------------------------------------------------------------------------------------------
	public function payload_deactivate() {

		$c = $this->connect_write();
		if ( is_array($c) && !empty($c['error']) ) {
			return $c;
		}

		$id = !empty($_POST['id']) ? sanitize_alnumdash( $_POST['id'] ) : '';
		$tbname = !empty($_POST['tbname']) ? sanitize_alnumdash( $_POST['tbname'] ) : '';
		$dt = !empty($_POST['dt']) ? intval( $_POST['dt'] ) : 0;

		if ( empty($id) || empty($tbname) || empty($dt) ) {
			return [ 'error' => 'insufficient data provided, cannot deactivate payload' ];
		}

		try {
			$stmt = $this->dbh->prepare('UPDATE cdb_iov_'.$tbname. ' SET dt = :dt WHERE id = :id');
			$stmt->execute([ 'dt' => $dt, 'id' => $id ]);
		} catch ( PDOException $e ) {
			$this->dbh->rollBack();
			return [ 'error' => $e->getMessage() ];
		}

		return [ 'id' => $id ];
	}

	public function payload_set() {

		$c = $this->connect_write();
		if ( is_array($c) && !empty($c['error']) ) {
			return $c;
		}

		$id = sanitize_alnumdash( $_POST['id'] );
		$pid = sanitize_alnumdash( $_POST['pid'] );
		$flavor = sanitize_alnum( $_POST['flavor'] );
		$ct = intval( $_POST['ct'] );
		$dt = intval( $_POST['dt'] );
		$bt = intval( $_POST['bt'] );
		$et = intval( $_POST['et'] );
		$run = intval( $_POST['run'] );
		$seq = intval( $_POST['seq'] );
		$fmt = sanitize_alnum($_POST['fmt']);
		$uri = sanitize_alnum($_POST['uri']);
		$tbname = sanitize_alnumscore($_POST['tbname']);
		$data = $_POST['data'];
		$data_size = intval( $_POST['data_size'] );

		if ( empty($uri) && strlen($data) == 0 ) {
			return [ 'error' => 'no URI and no data' ];
		}

		if ( empty($uri) ) {
			$uri = 'db://'.$tbname;
		}

		$this->dbh->beginTransaction();

		try {
			$stmt = $this->dbh->prepare('INSERT INTO cdb_iov_'.$tbname
				.' ( id, pid, flavor, ct, dt, bt, et, run, seq, fmt, uri ) VALUES ( :id, :pid, :flavor, :ct, :dt, :bt, :et, :run, :seq, :fmt, :uri )');
			$stmt->execute([ 'id' => $id, 'pid' => $pid, 'flavor' => $flavor,
				'ct' => $ct, 'dt' => $dt, 'bt' => $bt, 'et' => $et, 'run' => $run, 'seq' => $seq, 'fmt' => $fmt, 'uri' => $uri ]);
		} catch ( PDOException $e ) {
			$this->dbh->rollBack();
			return [ 'error' => $e->getMessage() ];
		}

		if ( strpos($uri, 'db://') === 0 && $data_size > 0 ) {
			// insert data
			try {
				$stmt = $this->dbh->prepare('INSERT INTO cdb_data_'.$tbname
					.' ( id, pid, ct, dt, data, size ) VALUES ( :id, :pid, :ct, :dt, :data, :size )');
				$stmt->execute([ 'id' => $id, 'pid' => $pid	, 'ct' => $ct, 'dt' => 0, 'data' => $data, 'size' => $data_size ]);
			} catch ( PDOException $e ) {
				$this->dbh->rollBack();
				return [ 'error' => $e->getMessage() ];
			}
		}

		$this->dbh->commit();

		return [ 'uuid' => $id ];
	}

	// -------------------------------------------------------------------------------------------------------------
	public function payload_get() {

		$c = $this->connect_read();
		if ( is_array($c) && !empty($c['error']) ) {
			return $c;
		}

		$tbname = !empty($_GET['tb']) ? sanitize_alnumscore($_GET['tb']) : '';
		$flavor = !empty($_GET['f']) ? sanitize_alnum($_GET['f']) : '';
		$mt = !empty($_GET['mt']) ? intval($_GET['mt']) : 0;
		$evt = !empty($_GET['et']) ? intval($_GET['et']) : 0;
		$run = !empty($_GET['run']) ? intval($_GET['run']) : 0;
		$seq = !empty($_GET['seq']) ? intval($_GET['seq']) : 0;

		$data = [];

		try {
			if ( $run > 0 ) {
				$query = 'SELECT id, pid, flavor, uri, bt, et, ct, dt, run, seq, fmt FROM cdb_iov_' . $tbname . ' WHERE '
            .'flavor = :flavor '
            .'AND run = :run '
            .'AND seq = :seq '
            .( $mt > 0 ? 'AND ct <= :mt1 ' : '' )
            .( $mt > 0 ? 'AND ( dt = 0 OR dt > :mt2 ) ' : '' )
            .'ORDER BY ct DESC LIMIT 1';
				$stmt = $this->dbh->prepare( $query );
				if ( $mt > 0 ) {
					$stmt->execute([ 'flavor' => $flavor, 'run' => $run, 'seq' => $seq, 'mt1' => $mt, 'mt2' => $mt ]);
				} else {
					$stmt->execute([ 'flavor' => $flavor, 'run' => $run, 'seq' => $seq ]);
				}
			} else {
         $query = 'SELECT id, pid, flavor, uri, bt, et, ct, dt, run, seq, fmt FROM cdb_iov_' . $tbname . ' WHERE '
            .'flavor = :flavor '
            .'AND bt <= :evt1 AND ( et = 0 OR et > :evt2 ) '
            .( $mt > 0 ? 'AND ct <= :mt1 ' : '' )
            .( $mt > 0 ? 'AND ( dt = 0 OR dt > :mt2 ) ' : '' )
            .'ORDER BY bt DESC LIMIT 1';

				$stmt = $this->dbh->prepare( $query );

				if ( $mt > 0 ) {
					$stmt->execute([ 'flavor' => $flavor, 'evt1' => $evt, 'evt2' => $evt, 'mt1' => $mt, 'mt2' => $mt ]);
				} else {
					$stmt->execute([ 'flavor' => $flavor, 'evt1' => $evt, 'evt2' => $evt ]);
				}
			}

			$data = $stmt->fetch(PDO::FETCH_ASSOC);
			if ( empty($data) ) {
				return [ 'error' => 'no results' ];
			}

		} catch ( PDOException $e ) {
			return [ 'error' => $e->getMessage() ];
		}

		// if no end time for the entry provided - find it
		if ( !empty($data) && $data['et'] == 0 ) {
			try {
  		  $query = 'SELECT bt FROM cdb_iov_' . $tbname . ' WHERE '
    		  .'flavor = :flavor '
      	  .'AND bt >= :evt1 '
        	.'AND ( et = 0 OR et < :evt2 )'
        	.( $mt > 0 ? 'AND ct <= :mt1 ' : '' )
        	.( $mt > 0 ? 'AND ( dt = 0 OR dt > :mt2 ) ' : '' )
        	.'ORDER BY bt ASC LIMIT 1';
				$stmt = $this->dbh->prepare( $query );
				if ( $mt > 0 ) {
					$stmt->execute([ 'flavor' => $flavor, 'evt1' => $evt, 'evt2' => $evt, 'mt1' => $mt, 'mt2' => $mt ]);
				} else {
					$stmt->execute([ 'flavor' => $flavor, 'evt1' => $evt, 'evt2' => $evt ]);
				}
				$edata = $stmt->fetch(PDO::FETCH_ASSOC);
				$bt = intval($edata['bt']);
				if ( $bt == 0 ) { $bt = PHP_INT_MAX; }
				$data['et'] = $bt;
			} catch ( PDOException $e ) {
				return [ 'error' => $e->getMessage() ];
			}
		}

		return [ 'payload' => $data ];
	}

	// -------------------------------------------------------------------------------------------------------------

}