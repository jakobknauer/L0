# Semantic Checks

The following semantic passes are being done:
- ModuleValidator: Checks if all module-level statements are declarations, and if the initializers are literals or functions.
- GlobalSymbolPass: Builds up global scope (with types), checks for duplicate declarations. (As soon as custom types can be declared, two passes will be necessary here.)
- Resolver: Creates local scopes, resolves uses of variables on function level, checks for duplicate declarations of local variables.
- TypeChecker: Checks if declared match actual types. Also operator overload resolution is done here (for + and * at this time)
- ReturnStatementPass: Checks existence and types of return statements in all branches of functions, and generates return statements for functions that return Unit, if not existing
