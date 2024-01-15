value get(k){
    lock; vlue = seqget(k);
    unlock;
    return v;
}

value inc(k,amount){
    lock;
    v = get(k); // interbloqueo con la funcion
    v += amount;
    put(k,v);
    unlock;
}
# Su ejecucion es especulativa, si los cambios especulativos al final son reales se marcan como definitivos en caso contrario se macan como invalidos.

 ** Problemas
    - Para hacerlo en hardware tiene que ser pocos datos.
    - Si hay muchos threads a la vez intentando realizar acciones, los threads tendrán que reintentar lo cual hara que la cpu trabaje bastante de manera innecesaria.




// RCU

p es un puntero a la memoria compartida

reader(){
    plocal = p;
}

writer(){
    plocal = p;
    pcopy = copy(plocal); // Se realiza una copia completa de la seccion compartida
    operaciones_en_memoria //
    // Se comprueba si p sigue siendo igual a plocal para ver si otro escritor lo ha cambiado, si ha sido cambiada se vuelve a empezar la funcion (do while?)
}

// Prioriza a los lectores y los escritores sobretodo si hay varios pueden tardar mucho en realizar sus cosas.

// Se suele usar para elementos que no se suelen escribir mucho pero si leer
// Por ejemplo una tabla de rutas sobretodo la tabla de ordenadores que solo se suele cambiar al tirar o levantar la tarjeta de red, aunque tambien se podria con la tabla de rutas del router.