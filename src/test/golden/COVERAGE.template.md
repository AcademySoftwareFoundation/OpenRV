# `<package>` — Migration Coverage Contract

**Purpose.** …

**Source of truth.** …

---

## Primary outcomes

Fill this **first**, before the behavior inventory or scenarios. See
[`VERIFICATION.md` § Primary outcomes](./VERIFICATION.md#primary-outcomes-required-per-package).

| # | User-visible outcome | Graph / property signal | Pixel discriminant | Scenario(s) | B | P |
|---|----------------------|-------------------------|--------------------|---------------|---|---|
| 1 | | | | | req | req if visible |

User approval required on this table before capture.

---

## File inventory

…

## Mu methods → Python unit tests

**Mandatory gate 5.** Inventory every Mu method/function before the migration loop ends.
Record observed behavior from the Mu sources; map each row to a Python unit test (or chain
test). See [`VERIFICATION.md` § Gate 5](./VERIFICATION.md#gate-5--python-unit-tests).

| Mu symbol | Kind | Recorded behavior (inputs → effects) | Python test | Status |
|-----------|------|--------------------------------------|-------------|--------|
| | fn / method / chain | | `unit/test_….py::…` | ⬜ |

- **Chain tests:** use one row for `A → B → C` when isolated tests would be meaningless;
  name the chain and list every Mu symbol it covers.
- **Status:** ✅ = test exists and passes; ⬜ = not yet covered. No ⬜ rows at migration done.

## Verification method

…

## Behavior inventory

…

## Dropped

…

## Scenarios

…
