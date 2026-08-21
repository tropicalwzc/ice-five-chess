# Pre-promotion binding regression

Command:

```sh
python3 tools/verify_five_star_ui_binding.py
```

Expected result before the product assignment changed: exit 1.

```text
FAIL: playable five-star method does not select the 5.8.1 factory
FAIL: playable five-star method still selects exact 5.4.1
```

The script itself passed `python3 -m py_compile`. This expected failure proves the regression distinguishes the old product binding from the authorized promotion.
