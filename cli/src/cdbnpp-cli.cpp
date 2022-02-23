
#include <npp/util/log.h>
#include <npp/util/result.h>

#include "cmd.h"
#include "config-commands.h"
#include "db-commands.h"
#include "file-commands.h"
#include "http-commands.h"
#include "memory-commands.h"

#include <iostream>

using namespace NPP::CDB;
using namespace NPP::Util;
using namespace NPP::CLI;

int main(int argc, const char *argv[]) {
	NPP::Util::Log::setError( &std::cerr );
	NPP::Util::Log::setInfo(  &std::cout );
	NPP::Util::Log::setDebug( &std::cout );

	Cmd cmds;

	cmds.registerCommand("config:show:adapters", "", "List all configured adapters", config_show_adapters );

	cmds.registerCommand("db:tags:create", "<path>", "Creates new tag/folder", db_tags_create );
	cmds.registerCommand("db:tags:deactivate", "<path> <time>", "Deactivates existing tag/folder using specified time", db_tags_deactivate );
	cmds.registerCommand("db:tags:list", "", "Lists all tags and structures", db_tags_list );
	cmds.registerCommand("db:struct:create", "<path>/<struct> <mode>", "Creates new struct using specified mode", db_struct_create );
	cmds.registerCommand("db:tables:create", "", "Initializes database tables", db_tables_create );
	cmds.registerCommand("db:tables:list", "", "Lists all database tables", db_tables_list );
	cmds.registerCommand("db:tables:drop", "", "Deletes all existing db tables", db_tables_drop );

	cmds.registerCommand("db:schema:set", "<path>/<struct> <file>", "Sets schema for <struct> from <file>", db_schema_set );
	cmds.registerCommand("db:schema:get", "<path>/<struct>", "Gets schema for <struct>", db_schema_get );
	cmds.registerCommand("db:schema:drop", "<path>/<struct>", "Deletes schema for <struct>", db_schema_drop );

	cmds.registerCommand("db:payload:set", "<path> <file> <b-time|run> <e-time|seq>", "Uploads a payload file (db-embedded)", db_payload_set );
	cmds.registerCommand("db:payload:getbytime", "<path> <e-time> <max-time>", "Requests a payload using event time and max entry time", db_payload_getbytime );
	cmds.registerCommand("db:payload:getbyrun", "<path> <run> <seq> <max-time>", "Requests a payload using run/seq and max entry time", db_payload_getbyrun );

	cmds.registerCommand("db:tags:export", "<output-file>", "Exports all tags into the output file (json)", db_tags_export );
	cmds.registerCommand("db:tags:import", "<input-file>", "Imports tags from the input file (json)", db_tags_import );

	cmds.registerCommand("file:tags:create", "<path>", "Creates a new tag/folder", file_tags_create );
	cmds.registerCommand("file:payload:setbytime", "<path> <file> <b-time> <e-time>", "Uploads a payload file using begin-end times", file_payload_setbytime );
	cmds.registerCommand("file:payload:setbyrun", "<path> <file> <run> <seq>", "Uploads a payload file using run/seq", file_payload_setbyrun );
	cmds.registerCommand("file:payload:getbytime", "<path> <b-time> <e-time> <max-time>", "Requests a payload using begin-end times", file_payload_getbytime );
	cmds.registerCommand("file:payload:getbyrun", "<path> <run> <seq> <max-time>", "Requests a payload using run/seq", file_payload_getbyrun );
	cmds.registerCommand("file:convert:json2ubjson", "<input-file> <output-file>", "Convert JSON file to UBJSON file", file_convert_json2ubjson );
	cmds.registerCommand("file:convert:ubjson2json", "<input-file> <output-file>", "Convert UBJSON file to JSON file", file_convert_ubjson2json );
	cmds.registerCommand("file:convert:json2cbor", "<input-file> <output-file>", "Convert JSON file to CBOR file", file_convert_json2cbor );
	cmds.registerCommand("file:convert:cbor2json", "<input-file> <output-file>", "Convert CBOR file to JSON file", file_convert_cbor2json );
	cmds.registerCommand("file:convert:json2msgpack", "<input-file> <output-file>", "Convert JSON file to MsgPack file", file_convert_json2msgpack );
	cmds.registerCommand("file:convert:msgpack2json", "<input-file> <output-file>", "Convert MsgPack file to JSON file", file_convert_msgpack2json );

	cmds.registerCommand("http:tags:list", "", "Lists all tags/folders", http_tags_list );
	cmds.registerCommand("http:tags:create", "<path>", "Creates new tag/folder", http_tags_create );
	cmds.registerCommand("http:tags:deactivate", "<path> <time>", "Deactivates tag/folder using specified time", http_tags_deactivate );
	cmds.registerCommand("http:struct:create", "<path>/<struct> <mode>", "Creates new struct using specified mode", http_struct_create );

	cmds.registerCommand("http:payload:set", "<path> <file|uri> <b-time|run> <e-time|seq>", "Uploads a payload file", http_payload_set );
	cmds.registerCommand("http:payload:getbyrun", "<path> <run> <seq> <max-time>", "Requests a payload using run/seq", http_payload_getbyrun );
	cmds.registerCommand("http:payload:getbytime", "<path> <e-time> <max-time>", "Requests a payload using event time", http_payload_getbytime );

	cmds.registerCommand("http:schema:set", "<path>/<struct> <file>", "Upload schema file for a given struct", http_schema_set );
	cmds.registerCommand("http:schema:get", "<path>/<struct>", "Request schema file for a given struct", http_schema_get );
	cmds.registerCommand("http:schema:drop", "<path>/<struct>", "Delete schema for a given struct", http_schema_drop );

	cmds.registerCommand("http:tables:list", "", "Lists all database tables", http_tables_list );
	cmds.registerCommand("http:tables:create", "", "Initializes database tables", http_tables_create );
	cmds.registerCommand("http:tables:drop", "", "Deletes all database tables", http_tables_drop );

	cmds.registerCommand("http:tags:export", "<file>", "Exports all tags and schemas into the file (json)", http_tags_export );
	cmds.registerCommand("http:tags:import", "<file>", "Imports tags and schemas from file (json)", http_tags_import );

	cmds.registerCommand("memory:test:setget", "", "Self-tests memory adapter", memory_test_setget );

	cmds.process( argc, argv );

	return EXIT_SUCCESS;
}
