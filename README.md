# Avaliação de Desempenho e Escalabilidade de um Operador Laplaciano Paralelo

Este trabalho apresenta a avaliação de desempenho e escalabilidade de uma implementação paralela do operador diferencial Laplaciano com máscara de dimensão **15 × 15**. O algoritmo foi desenvolvido em baixo nível na linguagem **C** utilizando a API **POSIX Threads (Pthreads)** com uma estratégia de divisão de dados por fatiamento horizontal (*stripwise*).

Os experimentos foram conduzidos em uma arquitetura **Intel Core i7-1165G7** utilizando uma imagem **PGM** de alta resolução (**4000 × 4000 pixels**).

Os resultados demonstraram que o *speedup* real satura em **3,23×**, limitado pelo número de núcleos físicos da CPU e pela capacidade da memória cache **L3**. Acima de **8 threads**, a eficiência decai até **1,17%**, evidenciando os impactos do *overhead* de troca de contexto e do fenômeno de *cache thrashing*.
