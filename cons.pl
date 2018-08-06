cons([], List, List).
cons([Head|Tail], List, [Head|AResult]) :-
	cons(Tail, List, AResult).
