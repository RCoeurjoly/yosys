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
    return;
    for (auto wire : module->wires())
      {
	if (wire->port_output)
	  for (auto bit : sigmap(wire))
	    sigbit_actsignals[bit] = State::S1;
      }

    for (auto cell : module->cells())
      {
	if (cell->type.in(ID($mux), ID($pmux)))
	  {
	    int width = cell->getParam(ID::WIDTH).as_int();
	    int numports = cell->type == ID($mux) ? 2 : cell->getParam(ID::S_WIDTH).as_int() + 1;

	    SigSpec sig_a = sigmap(cell->getPort(ID::A));
	    SigSpec sig_b = sigmap(cell->getPort(ID::B));
	    SigSpec sig_s = sigmap(cell->getPort(ID::S));

	    for (int i = 0; i < numports; i++) {
	      SigSpec bits = i == 0 ? sig_a : sig_b.extract(width*(i-1), width);
	      for (int k = 0; k < width; k++) {
		tuple<Cell*, int, int> muxuser(cell, i, k);
		sigbit_muxusers[bits[k]].insert(muxuser);
	      }
	    }
	  }
	else
	  {
	    for (auto &conn : cell->connections()) {
	      if (!cell->known() || cell->input(conn.first))
		for (auto bit : sigmap(conn.second))
		  sigbit_actsignals[bit] = State::S1;
	    }
	  }
      }
  }

  void run(Cell *add)
  {
    log("Adding assert for $add cell %s.%s.\n", log_id(module), log_id(add));
    return;
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
