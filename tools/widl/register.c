/*
 * Generation of dll registration scripts
 *
 * Copyright 2010 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"
#include "typegen.h"
#include "typelib.h"

static int indent;

static const char *format_uuid( const uuid_t *uuid )
{
    static char buffer[40];
    sprintf( buffer, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
             uuid->Data1, uuid->Data2, uuid->Data3,
             uuid->Data4[0], uuid->Data4[1], uuid->Data4[2], uuid->Data4[3],
             uuid->Data4[4], uuid->Data4[5], uuid->Data4[6], uuid->Data4[7] );
    return buffer;
}

static const char *get_coclass_threading( const type_t *class )
{
    static const char * const models[] =
    {
        NULL,
        "Apartment", /* THREADING_APARTMENT */
        "Neutral",   /* THREADING_NEUTRAL */
        "Single",    /* THREADING_SINGLE */
        "Free",      /* THREADING_FREE */
        "Both",      /* THREADING_BOTH */
    };
    return models[get_attrv( class->attrs, ATTR_THREADING )];
}

static const type_t *find_ps_factory( const statement_list_t *stmts )
{
    const statement_t *stmt;

    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type == STMT_TYPE)
        {
            const type_t *type = stmt->u.type;
            if (type_get_type(type) == TYPE_COCLASS && !strcmp( type->name, "PSFactoryBuffer" ))
                return type;
        }
    }
    return NULL;
}

static void write_interface( const type_t *iface, const type_t *ps_factory )
{
    const uuid_t *uuid = get_attrp( iface->attrs, ATTR_UUID );
    const uuid_t *ps_uuid = get_attrp( ps_factory->attrs, ATTR_UUID );

    if (!uuid) return;
    if (!is_object( iface )) return;
    if (!type_iface_get_inherit(iface)) /* special case for IUnknown */
    {
        put_str( indent, "'%s' = s '%s'\n", format_uuid( uuid ), iface->name );
        return;
    }
    if (is_local( iface->attrs )) return;
    put_str( indent, "'%s' = s '%s'\n", format_uuid( uuid ), iface->name );
    put_str( indent, "{\n" );
    indent++;
    put_str( indent, "NumMethods = s %u\n", count_methods( iface ));
    put_str( indent, "ProxyStubClsid32 = s '%s'\n", format_uuid( ps_uuid ));
    indent--;
    put_str( indent, "}\n" );
}

static void write_interfaces( const statement_list_t *stmts, const type_t *ps_factory )
{
    const statement_t *stmt;

    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type == STMT_TYPE && type_get_type( stmt->u.type ) == TYPE_INTERFACE)
            write_interface( stmt->u.type, ps_factory );
    }
}

static void write_typelib_interface( const type_t *iface, const typelib_t *typelib )
{
    const uuid_t *typelib_uuid = get_attrp( typelib->attrs, ATTR_UUID );
    const uuid_t *uuid = get_attrp( iface->attrs, ATTR_UUID );
    unsigned int version = get_attrv( typelib->attrs, ATTR_VERSION );

    if (!uuid) return;
    if (!is_object( iface )) return;
    put_str( indent, "'%s' = s '%s'\n", format_uuid( uuid ), iface->name );
    put_str( indent, "{\n" );
    indent++;
    put_str( indent, "ProxyStubClsid = s '{00020424-0000-0000-C000-000000000046}'\n" );
    put_str( indent, "ProxyStubClsid32 = s '{00020424-0000-0000-C000-000000000046}'\n" );
    if (version)
        put_str( indent, "TypeLib = s '%s' { val Version = s '%u.%u' }\n",
                 format_uuid( typelib_uuid ), MAJORVERSION(version), MINORVERSION(version) );
    else
        put_str( indent, "TypeLib = s '%s'", format_uuid( typelib_uuid ));
    indent--;
    put_str( indent, "}\n" );
}

static void write_typelib_interfaces( const typelib_t *typelib )
{
    unsigned int i;

    for (i = 0; i < typelib->reg_iface_count; ++i)
        write_typelib_interface( typelib->reg_ifaces[i], typelib );
}

static int write_coclass( const type_t *class, const typelib_t *typelib )
{
    const uuid_t *uuid = get_attrp( class->attrs, ATTR_UUID );
    const char *descr = get_attrp( class->attrs, ATTR_HELPSTRING );
    const char *progid = get_attrp( class->attrs, ATTR_PROGID );
    const char *vi_progid = get_attrp( class->attrs, ATTR_VIPROGID );
    const char *threading = get_coclass_threading( class );
    unsigned int version = get_attrv( class->attrs, ATTR_VERSION );

    if (!uuid) return 0;
    if (typelib && !threading && !progid) return 0;
    if (!descr) descr = class->name;

    put_str( indent, "'%s' = s '%s'\n", format_uuid( uuid ), descr );
    put_str( indent++, "{\n" );
    if (threading) put_str( indent, "InprocServer32 = s '%%MODULE%%' { val ThreadingModel = s '%s' }\n",
                            threading );
    if (progid) put_str( indent, "ProgId = s '%s'\n", progid );
    if (typelib)
    {
        const uuid_t *typelib_uuid = get_attrp( typelib->attrs, ATTR_UUID );
        put_str( indent, "TypeLib = s '%s'\n", format_uuid( typelib_uuid ));
        if (!version) version = get_attrv( typelib->attrs, ATTR_VERSION );
    }
    if (version) put_str( indent, "Version = s '%u.%u'\n", MAJORVERSION(version), MINORVERSION(version) );
    if (vi_progid) put_str( indent, "VersionIndependentProgId = s '%s'\n", vi_progid );
    put_str( --indent, "}\n" );
    return 1;
}

static void write_coclasses( const statement_list_t *stmts, const typelib_t *typelib )
{
    const statement_t *stmt;

    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type == STMT_TYPE)
        {
            const type_t *type = stmt->u.type;
            if (type_get_type(type) == TYPE_COCLASS) write_coclass( type, typelib );
        }
    }
}

static void write_runtimeclasses_registry( const statement_list_t *stmts )
{
    const statement_t *stmt;
    const type_t *type;

    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type != STMT_TYPE) continue;
        if (type_get_type((type = stmt->u.type)) != TYPE_RUNTIMECLASS) continue;
        if (!get_attrp(type->attrs, ATTR_ACTIVATABLE) && !get_attrp(type->attrs, ATTR_STATIC)) continue;
        put_str( indent, "ForceRemove %s\n", format_namespace( type->namespace, "", ".", type->name, NULL ) );
        put_str( indent++, "{\n" );
        put_str( indent, "val 'DllPath' = s '%%MODULE%%'\n" );
        put_str( --indent, "}\n" );
    }
}

static int write_progid( const type_t *class )
{
    const uuid_t *uuid = get_attrp( class->attrs, ATTR_UUID );
    const char *descr = get_attrp( class->attrs, ATTR_HELPSTRING );
    const char *progid = get_attrp( class->attrs, ATTR_PROGID );
    const char *vi_progid = get_attrp( class->attrs, ATTR_VIPROGID );

    if (!uuid) return 0;
    if (!descr) descr = class->name;

    if (progid)
    {
        put_str( indent, "'%s' = s '%s'\n", progid, descr );
        put_str( indent++, "{\n" );
        put_str( indent, "CLSID = s '%s'\n", format_uuid( uuid ) );
        put_str( --indent, "}\n" );
    }
    if (vi_progid)
    {
        put_str( indent, "'%s' = s '%s'\n", vi_progid, descr );
        put_str( indent++, "{\n" );
        put_str( indent, "CLSID = s '%s'\n", format_uuid( uuid ) );
        if (progid && strcmp( progid, vi_progid )) put_str( indent, "CurVer = s '%s'\n", progid );
        put_str( --indent, "}\n" );
    }
    return 1;
}

static void write_progids( const statement_list_t *stmts )
{
    const statement_t *stmt;

    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type == STMT_TYPE)
        {
            const type_t *type = stmt->u.type;
            if (type_get_type(type) == TYPE_COCLASS) write_progid( type );
        }
    }
}

void write_regscript( const statement_list_t *stmts )
{
    const type_t *ps_factory;

    if (!do_regscript) return;
    if (do_everything && !need_proxy_file( stmts )) return;

    init_output_buffer();

    if (winrt_mode)
    {
        put_str( indent, "HKLM\n" );
        put_str( indent++, "{\n" );
        put_str( indent, "NoRemove Software\n" );
        put_str( indent++, "{\n" );
        put_str( indent, "NoRemove Microsoft\n" );
        put_str( indent++, "{\n" );
        put_str( indent, "NoRemove WindowsRuntime\n" );
        put_str( indent++, "{\n" );
        put_str( indent, "NoRemove ActivatableClassId\n" );
        put_str( indent++, "{\n" );
        write_runtimeclasses_registry( stmts );
        put_str( --indent, "}\n" );
        put_str( --indent, "}\n" );
        put_str( --indent, "}\n" );
        put_str( --indent, "}\n" );
        put_str( --indent, "}\n" );
    }
    else
    {
        put_str( indent, "HKCR\n" );
        put_str( indent++, "{\n" );

        put_str( indent, "NoRemove Interface\n" );
        put_str( indent++, "{\n" );
        ps_factory = find_ps_factory( stmts );
        if (ps_factory) write_interfaces( stmts, ps_factory );
        put_str( --indent, "}\n" );

        put_str( indent, "NoRemove CLSID\n" );
        put_str( indent++, "{\n" );
        write_coclasses( stmts, NULL );
        put_str( --indent, "}\n" );

        write_progids( stmts );
        put_str( --indent, "}\n" );
    }

    if (strendswith( regscript_name, ".res" ))  /* create a binary resource file */
    {
        add_output_to_resources( "WINE_REGISTRY", regscript_token );
        flush_output_resources( regscript_name );
    }
    else
    {
        FILE *f = fopen( regscript_name, "w" );
        if (!f) error( "Could not open %s for output\n", regscript_name );
        if (fwrite( output_buffer, 1, output_buffer_pos, f ) != output_buffer_pos)
            error( "Failed to write to %s\n", regscript_name );
        if (fclose( f ))
            error( "Failed to write to %s\n", regscript_name );
    }
}

void write_typelib_regscript( const statement_list_t *stmts )
{
    const statement_t *stmt;
    unsigned int count = 0;

    if (!do_typelib) return;
    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type != STMT_LIBRARY) continue;
        if (count && !strendswith( typelib_name, ".res" ))
            error( "Cannot store multiple typelibs into %s\n", typelib_name );
        else
            create_msft_typelib( stmt->u.lib );
        count++;
    }
    if (count && strendswith( typelib_name, ".res" )) flush_output_resources( typelib_name );
}

void output_typelib_regscript( const typelib_t *typelib )
{
    const uuid_t *typelib_uuid = get_attrp( typelib->attrs, ATTR_UUID );
    const char *descr = get_attrp( typelib->attrs, ATTR_HELPSTRING );
    const expr_t *lcid_expr = get_attrp( typelib->attrs, ATTR_LIBLCID );
    unsigned int version = get_attrv( typelib->attrs, ATTR_VERSION );
    unsigned int flags = 0;
    char id_part[12] = "";
    char *resname = typelib_name;
    expr_t *expr;

    if (is_attr( typelib->attrs, ATTR_RESTRICTED )) flags |= 1; /* LIBFLAG_FRESTRICTED */
    if (is_attr( typelib->attrs, ATTR_CONTROL )) flags |= 2; /* LIBFLAG_FCONTROL */
    if (is_attr( typelib->attrs, ATTR_HIDDEN )) flags |= 4; /* LIBFLAG_FHIDDEN */

    put_str( indent, "HKCR\n" );
    put_str( indent++, "{\n" );

    put_str( indent, "NoRemove Typelib\n" );
    put_str( indent++, "{\n" );
    put_str( indent, "NoRemove '%s'\n", format_uuid( typelib_uuid ));
    put_str( indent++, "{\n" );
    put_str( indent, "'%u.%u' = s '%s'\n",
             MAJORVERSION(version), MINORVERSION(version), descr ? descr : typelib->name );
    put_str( indent++, "{\n" );
    expr = get_attrp( typelib->attrs, ATTR_ID );
    if (expr)
    {
        sprintf(id_part, "\\%d", expr->cval);
        resname = strmake("%s\\%d", typelib_name, expr->cval);
    }
    put_str( indent, "'%x' { %s = s '%%MODULE%%%s' }\n",
             lcid_expr ? lcid_expr->cval : 0, pointer_size == 8 ? "win64" : "win32", id_part );
    put_str( indent, "FLAGS = s '%u'\n", flags );
    put_str( --indent, "}\n" );
    put_str( --indent, "}\n" );
    put_str( --indent, "}\n" );

    put_str( indent, "NoRemove Interface\n" );
    put_str( indent++, "{\n" );
    write_typelib_interfaces( typelib );
    put_str( --indent, "}\n" );

    put_str( indent, "NoRemove CLSID\n" );
    put_str( indent++, "{\n" );
    write_coclasses( typelib->stmts, typelib );
    put_str( --indent, "}\n" );

    write_progids( typelib->stmts );
    put_str( --indent, "}\n" );

    add_output_to_resources( "WINE_REGISTRY", resname );
}
