/* Copyright (c) 2002-2008 Dovecot Sieve authors, see the included COPYING file
 */

/* Extension fileinto 
 * ------------------
 *
 * Authors: Stephan Bosch
 * Specification: RFC5228
 * Implementation: full
 * Status: experimental, largely untested
 *
 */

#include "sieve-extensions.h"
#include "sieve-binary.h"
#include "sieve-commands.h"
#include "sieve-code.h"
#include "sieve-actions.h"
#include "sieve-validator.h"
#include "sieve-generator.h"
#include "sieve-interpreter.h"
#include "sieve-dump.h"
#include "sieve-result.h"

/* 
 * Forward declarations 
 */

static const struct sieve_command fileinto_command;
const struct sieve_operation fileinto_operation;
const struct sieve_extension fileinto_extension; 

/* 
 * Extension
 */

static int ext_my_id;

static bool ext_fileinto_load(int ext_id);
static bool ext_fileinto_validator_load(struct sieve_validator *validator);

const struct sieve_extension fileinto_extension = { 
	"fileinto", 
	ext_fileinto_load,
	ext_fileinto_validator_load, 
	NULL, NULL, NULL, NULL, NULL,
	SIEVE_EXT_DEFINE_OPERATION(fileinto_operation), 
	SIEVE_EXT_DEFINE_NO_OPERANDS	
};

static bool ext_fileinto_load(int ext_id) 
{
	ext_my_id = ext_id;
	return TRUE;
}

static bool ext_fileinto_validator_load(struct sieve_validator *validator)
{
	/* Register new command */
	sieve_validator_register_command(validator, &fileinto_command);

	return TRUE;
}

/* 
 * Fileinto command
 *
 * Syntax: 
 *   fileinto <folder: string>
 */

static bool cmd_fileinto_validate
	(struct sieve_validator *validator, struct sieve_command_context *cmd);
static bool cmd_fileinto_generate
	(const struct sieve_codegen_env *cgenv, struct sieve_command_context *ctx);

static const struct sieve_command fileinto_command = { 
	"fileinto", 
	SCT_COMMAND,
	1, 0, FALSE, FALSE, 
	NULL, NULL,
	cmd_fileinto_validate, 
	cmd_fileinto_generate, 
	NULL 
};

/* 
 * Fileinto operation 
 */

static bool ext_fileinto_operation_dump
	(const struct sieve_operation *op, 
		const struct sieve_dumptime_env *denv, sieve_size_t *address);
static bool ext_fileinto_operation_execute
	(const struct sieve_operation *op, 
		const struct sieve_runtime_env *renv, sieve_size_t *address); 

const struct sieve_operation fileinto_operation = { 
	"FILEINTO",
	&fileinto_extension,
	0,
	ext_fileinto_operation_dump, 
	ext_fileinto_operation_execute 
};

/* 
 * Validation 
 */

static bool cmd_fileinto_validate
(struct sieve_validator *validator, struct sieve_command_context *cmd) 
{ 	
	struct sieve_ast_argument *arg = cmd->first_positional;
	
	if ( !sieve_validate_positional_argument
		(validator, cmd, arg, "folder", 1, SAAT_STRING) ) {
		return FALSE;
	}
	
	return sieve_validator_argument_activate(validator, cmd, arg, FALSE);
}

/*
 * Code generation
 */
 
static bool cmd_fileinto_generate
(const struct sieve_codegen_env *cgenv, struct sieve_command_context *ctx) 
{
	sieve_operation_emit_code(cgenv->sbin, &fileinto_operation, ext_my_id);

	/* Emit line number */
    sieve_code_source_line_emit(cgenv->sbin, sieve_command_source_line(ctx));

	/* Generate arguments */
	return sieve_generate_arguments(cgenv, ctx, NULL);
}

/* 
 * Code dump
 */
 
static bool ext_fileinto_operation_dump
(const struct sieve_operation *op ATTR_UNUSED,
	const struct sieve_dumptime_env *denv, sieve_size_t *address)
{
	sieve_code_dumpf(denv, "FILEINTO");
	sieve_code_descend(denv);

	/* Source line */
    if ( !sieve_code_source_line_dump(denv, address) )
        return FALSE;

	if ( !sieve_code_dumper_print_optional_operands(denv, address) ) {
		sieve_binary_corrupt(denv->sbin, 
			"FILEINTO: failed to dump optional operands");
		return FALSE;
	}

	if ( !sieve_opr_string_dump(denv, address) ) {
		sieve_binary_corrupt(denv->sbin, "FILEINTO: failed to dump string operand");
		return FALSE;
	}
	
	return TRUE;
}

/*
 * Execution
 */

static bool ext_fileinto_operation_execute
(const struct sieve_operation *op ATTR_UNUSED,
	const struct sieve_runtime_env *renv, sieve_size_t *address)
{
	struct sieve_side_effects_list *slist = NULL; 
	string_t *folder;
	unsigned int source_line;
	int ret = 0;

	/* Source line */
    if ( !sieve_code_source_line_read(renv, address, &source_line) )
        return FALSE;
	
	if ( !sieve_interpreter_handle_optional_operands(renv, address, &slist) )
		return FALSE;

	if ( !sieve_opr_string_read(renv, address, &folder) ) {
		sieve_binary_corrupt(renv->sbin, "FILEINTO: failed to read string operand");
		return FALSE;
	}

	sieve_runtime_trace(renv, "FILEINTO action (\"%s\")", str_c(folder));

	ret = sieve_act_store_add_to_result
		(renv, slist, str_c(folder), source_line);

	return ( ret >= 0 );
}






