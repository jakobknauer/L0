# Semantic Checks

The following semantic checks are required (either directly to ensure a well-formed program; or as a requirement to other checks)
- Declare types (+ kind)
- Define types
- Declare global variables
- Typecheck globals (excl. function return values)
- Resolve types
- Resolve variables (i.e. usages of variables in functions)
- Typecheck functions
- Check function return values
- Check references

The following graph shows the dependencies between these tasks:

![Dependencies](semantic-checks.svg)
