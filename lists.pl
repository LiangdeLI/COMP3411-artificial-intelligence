    is_a_list([]).
    is_a_list(.(Head, Tail)) :-
        is_a_list(Tail).

head_tail([AHead|ATail], AHead, ATail).

% base case
    is_member(Element, list(Element, _)).
    
    % recursive case (to be completed!)
    is_member(Element, list(_, Tail))   :-  
	is_member(Element, Tail).
