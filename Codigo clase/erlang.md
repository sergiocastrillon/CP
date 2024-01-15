## Ejercicio 1 del boletin de Erlang

start()-> spawn(?MODULE, loop, [[]]).

register(n) ->
    N!{register, self()}.

send(N,Msg) ->
    N!{send,Msg}.


------ 
## Ps es una lista de procesos
loop(Ps) -> 
    Receive
        {register, from} -> 
            loop([from|Ps]);
        {send, Msg} -> 
            send_to_all(Msg, Ps),
            loop(Ps)


-----

send_to_all(_,[]) -> OK,
send_to_all(Msg,[P|Ps]) ->
    P!Msg,
    send_to_all(Msg,Ps).

## Version alternativa

send_to_all(Msg,Ps) ->
    Lists:foreach(Fun(P)->P!Msg End,Ps).






## Ejercicio 2

start() -> 
    spawn(?MODULE,loop,[[]]).


store(S,P)->
    S!{store,P}.

get(S,F) ->
    S!{get,F,self()},
    receive
        {get_reply,R} ->
            R
    END.


loop(Ps) ->
    receive
        {store,P} ->
            loop([P|Ps]),
        {get,f,from} ->
            case find(Ps,F) of
                NOT_FOUND ->
                    FROM!{get_reply,{error,no_product}},
                    loop(Ps);
                {ok,P,N_Ps} ->
                    FROM!{get_reply,{ok,P}},
                    loop(N_Ps);
            END.




find([],_) -> NOT_FOUND;
find([P|Ps],F,Acc) -> ## En Acc están los elementos ya procesados
    case F(P) of
        true -> {ok,P,Acc++Ps},
        _ ->
            find(Ps,F,[P|Acc]).
    END.












## Ejercicio 3 (Arboles)

height(tree) ->
    tree!{height,self()},
    receive
        {height_reply,H} ->
            H
    End.





loop(children)->
    receive
        {height,from} ->
            Ht = 1 + Lists.foldl(Fun(Hmax,T) -> MAX(Hmax,Height(T)) End, -1,Children)
            from!{Height_reply,Ht},
            loop(Children)
    End. ## Los end van al mismo nivel de tab que loop??