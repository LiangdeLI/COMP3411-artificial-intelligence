% Liangde Li
% z5077896
% comp3411 Assignment 1

% Question 1 ================================================
sumsq_neg([], 0).

% if Head is non-negative, do not add it
sumsq_neg([Head|Tail], Sum) :-
    Head >= 0,
    sumsq_neg(Tail, Sum).

% if Head is negative, add the square of it
sumsq_neg([Head|Tail], Sum) :-
    Head < 0,
    sumsq_neg(Tail, PartSum),
    Sum is Head * Head + PartSum.



% Question 2 ================================================

% base case for either Who_List or What_List is empty
all_like_all(_,[]).

all_like_all([],_).

% for each person, test whether the person likes each thing
all_like_all([Head|Tail], [First|Rest]) :-
    likes(Head, First),
    all_like_all([Head], Rest),
    all_like_all(Tail, [First|Rest]).    



% Question 3 ================================================

% when N>M, add [N, sqrt(N)], and decrement N to call next step
addOnePair(N, M, List, [[N, Sqrt]|ResultList]) :-
    N >M,
    Sqrt is sqrt(N),
    Q is N - 1,
    addOnePair(Q, M, List, ResultList).

addOnePair(N, M, List, [[N, Sqrt]|List]) :-
    N = M,
    Sqrt is sqrt(N).

sqrt_table(N, M, Result) :-
	addOnePair(N, M, [], Result).



% Question 4 ================================================

% for the last element in the List, add it to NewList
chop_up([Head], [Head]).

% if Head First Second(first three elements in List) are successive
chop_up([Head|[First|[Second|Tail]]], [ [Head|Then] | Rest]) :-
	Head is First - 1,
	First is Second - 1,
	chop_up([First|[Second|Tail]], [ [First|Then] | Rest]).

% if only Head First are successive
chop_up([Head|[First|[Second|Tail]]], [[Head, First]|Rest]) :-
	Head is First - 1,
    First =\= Second -1,
	chop_up([First|[Second|Tail]], [First|Rest]).

% if only First Second are successive
chop_up([Head|[First|[Second|Tail]]], [Head | [[First|Then]|Rest]]) :-
	Head =\= First - 1,
	First is Second - 1,
	chop_up([First|[Second|Tail]], [[First|Then] | Rest]).

% if none is successive
chop_up([Head|[First|[Second|Tail]]], [Head| [First|Rest]]) :-
	Head =\= First - 1,
	First =\= Second -1,
	chop_up([First|[Second|Tail]], [First | Rest]).

% only two elements in the List, and successive
chop_up([Head,Second], [[Head,Second]]) :-
	Head is Second - 1.

% only two elements in the List, but not successive
chop_up([Head,Second], [Head, Second]) :-
    Head =\= Second - 1.



% Question 5 ================================================

% if the number is z, Eval is Value
tree_eval(Value, tree(empty, z, empty), Value).

% if the leaf is Num, the Eval is Num
tree_eval(_Value, tree(empty, Num, empty), Num).

% if operator is +, add left and right
tree_eval(Value, tree(Tree1, '+', Tree2), Eval) :-
	tree_eval(Value, Tree1, Eval1),
	tree_eval(Value, Tree2, Eval2),
	Eval is Eval1 + Eval2.

% if operator is -, delete left and right
tree_eval(Value, tree(Tree1, '-', Tree2), Eval) :-
	tree_eval(Value, Tree1, Eval1),
	tree_eval(Value, Tree2, Eval2),
	Eval is Eval1 - Eval2.

tree_eval(Value, tree(Tree1, '/', Tree2), Eval) :-
	tree_eval(Value, Tree1, Eval1),
	tree_eval(Value, Tree2, Eval2),
	Eval is Eval1 / Eval2.

tree_eval(Value, tree(Tree1, '*', Tree2), Eval) :-
	tree_eval(Value, Tree1, Eval1),
	tree_eval(Value, Tree2, Eval2),
	Eval is Eval1 * Eval2.