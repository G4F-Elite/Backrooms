#pragma once
// limen_props.h is kept in parts so that no file passes 500 significant lines (build.bat).
// They are one header: each part leans on the ones before it.

#include "limen_props_rules.h"  // placement guards, theme picking, liminal prop rules
#include "limen_props_suites.h"  // room suites, themed POIs, structured islands
#include "limen_props_halls.h"  // hall zones, bridges, chunk themes, processions, quirk props
#include "limen_props_content.h"  // vignettes, chunk content rebuild, updateMapContent
