function Inspector(data) {
	"use strict";

	var samples = null;
	var functions = Object.create(null);	// empty object without prototype.

	function enter(calls, id, tick) {
		var call = {
			hits: 1,
			self: 0,
			total: null,
			enter: tick,
			leave: null,
			memory: null,
			caller: null,
			callTree: null,
			func: functions[id] || {}
		};

		var samples = calls[0];

		// update global execution time
		if (samples.enter != null && samples.enter > tick) {
			samples.enter = tick;
		}

		if (samples.samples != null) {
			if (functions[id] == null) {
				console.log('invalid offset: ' + id);
			}
			var sample = samples.samples[id] || (samples.samples[id] = {
					func: functions[id] || {},
					hits: 0,
					self: 0,
					total: 0,
					get others() {
						return this.total - this.self;
					},
					callers: {},
					callees: {}
				});
			if (sample.hits != null) {
				sample.hits += 1;
			}
		}
		return call;
	}

	function leave(calls, caller, callee, tick, memory) {
		var j, recursive = false;
		var time = tick - callee.enter;
		var calleeId = callee.func.offs;
		var callerId = caller.func && caller.func.offs;

		for (j = 1; j < calls.length; ++j) {
			if (calleeId === calls[j].func.offs) {
				if (calls[j].hits != null) {
					calls[j].hits += 1;
				}
				recursive = true;
			}
		}

		if (callee.caller !== null ||
			callee.memory !== null ||
			callee.leave !== null ||
			callee.total !== null
		) {
			throw "invalid state"
		}
		callee.leave = tick;
		callee.caller = caller;
		callee.memory = memory;
		callee.total = time;

		if (callee.self != null) {
			callee.self = callee.total;
			if (callee.callTree != null) {
				for (j = 0; j < callee.callTree.length; ++j) {
					callee.self -= callee.callTree[j].total;
				}
			}
		}
		var samples = calls[0];
		if (samples.excluded == null || samples.excluded.indexOf(callee.name) < 0) {
			caller.callTree = (caller.callTree || []);
			caller.callTree.push(callee);
		}

		// update global execution time
		if (samples.leave != null && samples.leave < tick) {
			samples.leave = tick;
		}

		// compute global information
		if (samples.samples != null) {
			var sample = samples.samples[calleeId] || {};
			// update time spent in function
			if (sample.total != null && !recursive) {
				sample.total += time;
			}
			if (sample.self != null) {
				sample.self += time;
			}

			// update list of functions which have calls to this function
			if (callerId != null) {
				if (sample.callers != null) {
					sample.callers[callerId] = (sample.callers[callerId] || 0) + 1;
				}

				sample = samples.samples[callerId];
				// update time spent in function
				if (sample.self != null) {
					sample.self -= time;
				}

				// update list of functions invoked from this function
				if (sample.callees != null) {
					sample.callees[calleeId] = (sample.callees[calleeId] || 0) + 1;
				}
			}
		}
	}

	function buildCallTree() {
		var recSize = 2;
		var funIndex = 1;
		var tickIndex = 0;
		var heapIndex = -1;

		if (data.profile == null) {
			data.profile = {
				callTree: [],
				ticksPerSec: -1,
				functions: [],
				functionCount: 0,
				statements: [],
				statementCount: 0
			};
		}

		if (data.profile.callTreeData != null) {
			recSize = data.profile.callTreeData.length;
			funIndex = data.profile.callTreeData.indexOf("ctFunIndex");
			tickIndex = data.profile.callTreeData.indexOf("ctTickIndex");
			heapIndex = data.profile.callTreeData.indexOf("ctHeapIndex");
			if (!(recSize > 0)) {
				throw "callTreeData header is invalid";
			}
		}

		var i, caller, callee, calls = [{
			enter: +Infinity,
			leave: -Infinity,
			samples: {},
			callTree: null,
			//excluded: ['Halt', 'ToDays'],
			ticksPerSec: data.profile.ticksPerSec
		}];

		for (i = 0; i < data.profile.functions.length; ++i) {
			var symbol = data.profile.functions[i];
			if (symbol.proto == null) {
				symbol.proto = symbol[''];
			}
			functions[symbol.offs] = symbol;
		}

		var lastTick = 0;
		for (i = 0; i < data.profile.callTree.length; i += recSize) {
			var offs = data.profile.callTree[i + funIndex];
			var tick = tickIndex < 0 ? undefined : data.profile.callTree[i + tickIndex];
			var heap = heapIndex < 0 ? undefined : data.profile.callTree[i + heapIndex];

			// if a function executes in zero time we are in trouble displaying, so correct it to 1 unit
			// this happens with old compilers, with low resolution `clock()` function
			if (tick <= lastTick) {
				lastTick += 1;
				tick = lastTick;
			}
			lastTick = tick;
			if (offs !== -1) {
				callee = enter(calls, offs, tick);
				calls.push(callee);
			}
			else {
				callee = calls.pop();
				caller = calls[calls.length - 1];
				leave(calls, caller, callee, tick, heap);
				Object.freeze(callee);
			}
		}

		while (calls.length > 0) {
			samples = calls.pop();
			if (calls.length > 1) {
				caller = calls[calls.length - 1];
				leave(calls, caller, samples, undefined);
			}
			Object.freeze(samples);
		}
	}

	function getFiltered(checkSymbol) {
		var result = '';
		if (data == null) {
			return 'No data available!';
		}

		function symToString(sym, qual, type, doc) {
			var result = '';
			if (sym.owner && qual === true) {
				result += sym.owner + '.';
			}
			result += sym.name;
			if (sym.args !== undefined) {
				result += '(';
				for (var i = 0; i < sym.args.length; i += 1) {
					if (i > 0) {
						result += ', ';
					}
					result += symToString(sym.args[i], false, true, doc);
				}
				result += ')';
			}
			if (type === true) {
				result += ": " + sym.type;
			}
			if (doc === true && sym.doc != null) {
				result += "; " + sym.doc.trim().replace(/\n/g, '\\n');
			}
			return result;
		}

		var api = data.symbols || [];
		for (var i = 0; i < api.length; i += 1) {
			var sym = api[i];
			if (!checkSymbol(sym)) {
				continue;
			}
			result += symToString(sym, true, true, true);
			result += '\n';
		}
		return result;
	}

	if (data != null) {
		buildCallTree();
	}

	return {
		getSymbols: function(types, funcs, vars, defs) {
			return getFiltered(function (sym) {
				if (types && sym.kind === 'typename') {
					return true;
				}
				if (funcs && sym.kind === 'function') {
					return true;
				}
				if (vars && sym.kind === 'variable') {
					return true;
				}
				if (defs && sym.kind === 'inline') {
					return true;
				}
				//console.log("symbol was filtered: " + sym.proto);
				console.log("symbol[" + sym.kind + "] was filtered: " + sym.proto);
				return false;
			});
		},
		getSamples: function() {
			return samples;
		}
	}
}
