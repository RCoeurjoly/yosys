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

  // get<0> ... mux cell
  // get<1> ... mux port index
  // get<2> ... mux bit index
  dict<SigBit, pool<tuple<Cell*, int, int>>> sigbit_muxusers;

  dict<SigBit, SigBit> sigbit_actsignals;
  dict<SigSpec, SigBit> sigspec_actsignals;
  dict<tuple<Cell*, int>, SigBit> muxport_actsignal;

  AssertaddWorker(Module *module) :
    module(module), sigmap(module)
  {
    vector<Wire*> input_wires;
    vector<Wire*> non_input_public_wires;
    for (auto wire : module->wires())
      {
	if (!wire->name.isPublic())
	  continue;
	if (wire->port_input)
	  input_wires.push_back(wire);
	else
	  non_input_public_wires.push_back(wire);
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
    for (auto wire: input_wires)
      addBoundCheck(wire, &RTLIL::Module::addAssume);
    for (auto wire: non_input_public_wires)
      addBoundCheck(wire, &RTLIL::Module::addAssert);
  }

  typedef RTLIL::Cell* (RTLIL::Module::*AddFunc)(RTLIL::IdString, const RTLIL::SigSpec&, const RTLIL::SigSpec&, const std::string&);

  void addBoundCheck(Wire *wire, AddFunc addFunc) {
    // We need both bounds to insert the property
    if (wire->attributes.count(ID(left_bound)) == 0 || wire->attributes.count(ID(right_bound)) == 0)
      return;
    const RTLIL::Const left_bound = wire->attributes[ID(left_bound)];
    const RTLIL::Const right_bound = wire->attributes[ID(right_bound)];
    bool is_signed = false;
    if (left_bound.as_int() < 0 || right_bound.as_int() < 0)
      {
	is_signed = true;
      }
    SigSpec ge_than_left_bound = module->Ge(NEW_ID, wire, left_bound, is_signed);
    SigSpec le_than_right_bound = module->Le(NEW_ID, wire, right_bound, is_signed);
    SigSpec within_bounds_condition = module->LogicAnd(NEW_ID, ge_than_left_bound, le_than_right_bound);
    SigSpec assert_en = State::S1;

    Cell *cell = (module->*addFunc)(NEW_ID, within_bounds_condition, assert_en, "");
    if (wire->attributes.count(ID::src) != 0)
      cell->attributes[ID::src] = wire->attributes.at(ID::src);
  }
  
  void run(Cell *add)
  {
    log("Adding assert for $add cell %s.%s.\n", log_id(module), log_id(add));

    const int asign = add->getParam(ID::A_SIGNED).as_int();
    const int bsign = add->getParam(ID::B_SIGNED).as_int();

    SigSpec a = add->getPort(ID::A);
    SigSpec b = add->getPort(ID::B);
    SigSpec y = add->getPort(ID::Y);

    if (asign == 1 && bsign == 1) {
      log("Signed addition for $add cell %s.%s.\n", log_id(module), log_id(add));
      
      const int awidth = add->getParam(ID::A_WIDTH).as_int();
      const int bwidth = add->getParam(ID::B_WIDTH).as_int();
      const int ywidth = add->getParam(ID::Y_WIDTH).as_int();

      // Extract the sign bits of a, b, and y
      SigSpec sign_a = a.extract(awidth - 1);
      SigSpec sign_b = b.extract(bwidth - 1);
      SigSpec sign_y = y.extract(ywidth - 1);

      // Check if the signs of a and b are equal
      SigSpec same_sign_ab = module->Eq(NEW_ID, sign_a, sign_b);

      // Check if the sign of y is different from the sign of a (or b, since they are the same)
      SigSpec different_sign_ay = module->Ne(NEW_ID, sign_y, sign_a);

      // The assertion condition: same_sign_ab implies not different_sign_ay
      SigSpec overflow_condition = module->LogicAnd(NEW_ID, same_sign_ab, different_sign_ay);
      SigSpec assert_a = module->LogicNot(NEW_ID, overflow_condition);

      // Set enable signal to a constant 1
      SigSpec assert_en = State::S1;

      // Create the assert cell
      Cell *assert_cell = module->addAssert(NEW_ID, assert_a, assert_en);

      // Copy the source attribute if present
      if (add->attributes.count(ID::src) != 0)
	assert_cell->attributes[ID::src] = add->attributes.at(ID::src);
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

	for (auto cell : add_cells) {
	  // worker.run(cell);
	}
      }

  }
} AssertaddPass;

PRIVATE_NAMESPACE_END
