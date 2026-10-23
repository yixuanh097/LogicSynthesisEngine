# Logic Synthesis Engine

This Logic Synthesis Engine program analyzes combinational circuits given in either boolean algebra or graphical format (specifically, BLIF). It has the following functionalities performed in order:

1) It inteprets the boolean expression or BLIF file, parses input/output names,  generates truth table entries for all input combinations represented in a bitstream. We also provided a truth table exporter that outputs the truth table to a file for each output variable.

2) Using this bitstream, it reports the canonical SOP/POS forms, optimizes the number of literals (area) using the tabular method, reports the minimized SOP/POS forms as well as the number of literals saved, and logs the statistics involved in the optimization (number of prime implicants, essential prime implicants, on-set minterms, and off-set maxterms). The same procedure can be performed for the inverse function, but only its canonical form is reported.

3) In addition, it checks for static hazards, eliminates them by introducing redundancies, and reports the hazard-free version. 

4) It creates a synthesizable structural verilog description for each output using the minimized SOP expression in step 2, and writes it to a file that can be loaded to an FPGA.


## Folder Layout

The program separates input parsing, Boolean analysis, Verilog generation, and main control flow into serveral components:

- `main.cpp`: coordinates commandline parsing, file loading, selects the input format, runs the analysis, prints results to stdout, and writes output to files.
- `parser.cpp` and `parser.h`: parse given boolean expressions and BLIF  networks (given in .txt and .blif files), evaluate circuits, generate truth-table bitstreams, and provide the truth table exporter function.
- `analyzer.cpp` and `analyzer.h`: from given bitstreams, generate canonical and minimized SOP/POS, count implicants and minterms, performs the tabular method to reduce literals, quantify the improvement, and produce hazard-free SOP.
- `verilog_generation.cpp` and `verilog_generation.h`: convert a minimized SOP expression given in bitstream into structural Verilog using `not`, `and`, and `or` gate primitives.

Overall workflow:
For a BLIF input, the parser evaluates each declared output over all primary-input combinations. For a text input, each nonempty line is parsed as an SOP output expression. The resulting bitstreams are passed to the analyzer; the minimized SOP is then used to generate that output's Verilog module.

There is an example folder containing a simple adder module in both .txt and .blif formats, and also includes output files, verilog modules, and truth table outputs from running the program on adder.blif.

## Requirements

- A C++ compiler with C++17 support.
- GNU Make.

## Build

From the repository root:

```sh
make
```

This compiles the parser, analyzer, and Verilog generator, then links them into the `main` executable.

To remove the generated object files and executable:

```sh
make clean
```

## Run

The command takes two arguments: an input file and an analysis output file.

```sh
./main <input-file> <analysis-output-file>
```

For example:

```sh
./main Example/adder.blif Example/out.txt
```

or:

```sh
./main Example/adder.txt Example/out.txt
```

Analysis is written to the named output file and also printed to the terminal. For each output, the program additionally creates a structural Verilog file named `<output-name>.v` and a truth table file named `<output-name>.txt` in the current working directory. For the adder example, the generated files are `sum.v`, `cout.v`, `sum.txt`, and `cout.txt`, 

## Input formats

### SOP text files

Use one output expression per line. The output name is optional; when omitted, the program uses `Out`. For example,

```text
f = a'b + ac
g = ab + c'
```

Boolean expression notation:
- `+` means OR between product terms.
- Adjacent literals mean AND.
- A trailing apostrophe means NOT (for example, `a'`).
- Input variables in text SOP expressions must be single letters.
- Special characters other than and, or, not are not supported

### BLIF files

The program follows the BLIF format convention. Yse the combinational `.names` format with `.model`, `.inputs`, `.outputs`, truth-table rows, and `.end`. For example:

```blif
.model simple
.inputs a b
.outputs f
.names a b f
11 1
.end
```

The program chooses BLIF parsing when the input filename ends in `.blif`. Sequential BLIF elements and other advanced BLIF directives are not supported.

## Analysis and generated files

For each output, the analysis report includes:

- Canonical SOP and POS for the function and its complement.
- Minimized SOP and POS and literal-savings counts.
- Prime implicant, essential prime implicant, on-set minterm, and off-set maxterm counts.
- Whether a static hazard was detected, and a hazard-free SOP if one was detected.
- A truth table outputted to a text file.

## Structural Verilog output

The program writes one Verilog file per circuit output in the current working directory. Each file contains a synthesizable combinational module named after the output. Its input ports are the circuit's input nets, and its output port is the corresponding output net. The module is structural gate-level Verilog generated from the minimized SOP:

- `not` gates invert complemented input literals.
- `and` gates form each product term.
- An `or` gate combines the product terms into the output.
- Internal wires connect the gates, and the default name for these connections are `w_<number>`

For example, an SOP such as `f = a' b + a c` is represented using an inverter for `a`, two AND gates for the products, and an OR gate driving `f`.

