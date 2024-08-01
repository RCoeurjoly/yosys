/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "kernel/yosys.h"
#include "kernel/sigtools.h"

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct AssertaddWorker
{
  Module *module;
  SigMap sigmap;

  bool flag_noinit;
  bool flag_always;

  // get<0> ... mux cell
  // get<1> ... mux port index
  // get<2> ... mux bit index
  dict<SigBit, pool<tuple<Cell*, int, int>>> sigbit_muxusers;

  dict<SigBit, SigBit> sigbit_actsignals;
  dict<SigSpec, SigBit> sigspec_actsignals;
  dict<tuple<Cell*, int>, SigBit> muxport_actsignal;

  AssertaddWorker(Module *module, bool flag_noinit = false, bool flag_always = false) :
    module(module), sigmap(module), flag_noinit(flag_noinit), flag_always(flag_always)
  {
    for (auto wire : module->wires())
      {
	if (wire->port_output)
	  for (auto bit : sigmap(wire))
	    sigbit_actsignals[bit] = State::S1;
      }
    
    for (auto cell : module->cells())
      {
	if (cell->type.in(ID($add)))
	  {
	    log("Found $add cell.\n");
	  }
	else
	  {
	    
	  }
      }
  }

  void run(Cell *add)
  {
    log("Adding assert for $add cell %s.%s.\n", log_id(module), log_id(add));

    const int awidth = add->getParam(ID::A_WIDTH).as_int();
    const int bwidth = add->getParam(ID::B_WIDTH).as_int();
    const int ywidth = add->getParam(ID::Y_WIDTH).as_int();

    const int asign = add->getParam(ID::A_SIGNED).as_int();
    const int bsign = add->getParam(ID::B_SIGNED).as_int();

    SigSpec a = add->getPort(ID::A);
    SigSpec b = add->getPort(ID::B);
    SigSpec y = add->getPort(ID::Y);

    if (asign == 1 && bsign == 1) {
      log("Signed addition for $add cell %s.%s.\n", log_id(module), log_id(add));
    }
    else {
      log("Unsigned addition for $add cell %s.%s.\n", log_id(module), log_id(add));
      // Ensure the result c is greater than or equal to a and b
      SigSpec ge_a = module->Ge(NEW_ID, y, a);
      SigSpec ge_b = module->Ge(NEW_ID, y, b);
      SigSpec assert_a = module->LogicAnd(NEW_ID, ge_a, ge_b);

      // Set enable signal to a constant 1
      SigSpec assert_en = State::S1;

      // Create the assert cell
      Cell *assert_cell = module->addAssert(NEW_ID, assert_a, assert_en);

      // Copy the source attribute if present
      if (add->attributes.count(ID::src) != 0)
	assert_cell->attributes[ID::src] = add->attributes.at(ID::src);
    }
   
  }

};

struct AssertaddPass : public Pass {
  AssertaddPass() : Pass("assertadd", "adds asserts for add cells, checking overflow") { }
  void help() override
  {
    //   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
    log("\n");
    log("    assertadd [options] [selection]\n");
    log("\n");
    log("This command adds asserts to the design that assert that all add cells\n");
    log("($add cells) do not overflow.\n");
    log("\n");
  }
  void execute(std::vector<std::string> args, RTLIL::Design *design) override
  {
    log_header(design, "Executing ASSERTADD pass (add asserts for $add cells).\n");

    size_t argidx;
    for (argidx = 1; argidx < args.size(); argidx++)
      {
	break;
      }
    extra_args(args, argidx, design);

    for (auto module : design->selected_modules())
      {
	AssertaddWorker worker(module);
	vector<Cell*> add_cells;

	for (auto cell : module->selected_cells())
	  if (cell->type == ID($add))
	    add_cells.push_back(cell);

	for (auto cell : add_cells)
	  worker.run(cell);
      }

  }
} AssertaddPass;

PRIVATE_NAMESPACE_END
