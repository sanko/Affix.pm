/*  You may distribute under the terms of either the GNU General Public License
 *  or the Artistic License (the same terms as Perl itself)
 *
 *  (C) Paul Evans, 2021-2022 -- leonerd@leonerd.org.uk
 */
#define PERL_NO_GET_CONTEXT

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "object_pad.h"

static bool struct_apply(pTHX_ ClassMeta *classmeta, SV *hookdata, SV **hookdata_ptr, void *_funcdata)
{ mop_class_set_superclass(classmeta, sv_2mortal(newSVpvs("Dyn::Sugar::Struct")));
  mop_class_apply_attribute(classmeta, "strict", sv_2mortal(newSVpvs("params")));
    warn("apply");
  return TRUE;
}

static void struct_post_add_field(pTHX_ ClassMeta *classmeta, SV *hookdata, void *_funcdata, FieldMeta *fieldmeta)
{
    warn("post_add_field");
  if(mop_field_get_sigil(fieldmeta) != '$')
    return;

  mop_field_apply_attribute(fieldmeta, "param", NULL);
  mop_field_apply_attribute(fieldmeta, "mutator", NULL);
}

static const struct ClassHookFuncs struct_hooks = {
  .ver   = OBJECTPAD_ABIVERSION,
  .flags = OBJECTPAD_FLAG_ATTR_NO_VALUE,
  .permit_hintkey = "Dyn::Sugar::Struct/Struct",

  .apply          = &struct_apply,
  .post_add_field = &struct_post_add_field,
};

MODULE = Dyn::Sugar::Struct    PACKAGE = Dyn::Sugar::Struct

BOOT:
  register_class_attribute("Struct", &struct_hooks, NULL);
