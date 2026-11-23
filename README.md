# Biblioteca de Grafos em C++

Biblioteca para manipulação de grafos **direcionados ou não direcionados**, **com ou sem pesos**, escrita em C++17, junto com os programas usados nos estudos de caso da disciplina COS 242 (Teoria dos Grafos).

## Funcionalidades

| Área | O que a biblioteca faz |
|---|---|
| Representação | Lista de adjacência ou matriz de adjacência (escolha do usuário) |
| Tipos de grafo | Não direcionado ou direcionado (parâmetro na leitura); sem pesos ou com pesos (inclusive negativos) |
| Informações básicas | Número de vértices e arestas; grau mínimo, máximo, médio e mediana |
| Buscas | BFS e DFS, com árvore de busca (pai e nível de cada vértice) |
| Distâncias | Distância (em arestas) entre dois vértices; diâmetro exato e aproximado |
| Componentes | Componentes conexas (fracamente conexas em grafos direcionados) |
| Caminhos mínimos | **Dijkstra** (com vetor ou com heap), **Bellman-Ford** (todos os vértices até um destino) e **Floyd-Warshall** (todos os pares) |

## Estrutura do projeto

| Arquivo | Descrição |
|---|---|
| `Graph.hpp` / `Graph.cpp` | A biblioteca (classe `Graph`) |
| `MemoryUtils.hpp` | Leitura da memória residente (RSS) do processo, via `/proc/self/status` (Linux) |
| `main.cpp` | Programa de demonstração: gera os arquivos de saída da biblioteca |
| `case_study.cpp` | Estudos de caso da Parte 1 (memória, tempo de BFS/DFS, componentes, diâmetro) |
| `case_study2.cpp` | Estudos de caso da Parte 2 (Dijkstra com vetor e com heap) |
| `case_study3.cpp` | Estudos de caso da Parte 3 (grafos direcionados, Bellman-Ford, Floyd-Warshall) |
| `test_random.cpp` | Teste automático em grafos aleatórios pequenos |
| `Makefile` | Compilação |

## Requisitos e compilação

Precisa de `g++` com suporte a C++17 e `make`. No Ubuntu/Debian: `sudo apt install build-essential`.

```
make
```

Isso gera quatro executáveis: `graph_program`, `case_study`, `case_study2` e `case_study3`. Para compilar só um deles, use por exemplo `make case_study3`.

> **Atenção:** `make clean` apaga também todos os arquivos `*.txt` da pasta. Se seus grafos de entrada estiverem na mesma pasta, guarde-os em outro lugar.

## Formato do arquivo de entrada

Arquivo de texto:

```
<número de vértices n>
<u> <v> [peso]
<u> <v> [peso]
...
```

- Os vértices são numerados de **1 a n**.
- Cada linha seguinte é uma aresta. O formato é decidido pela primeira aresta: **duas colunas** (`u v`) formam um grafo sem pesos, em que cada aresta vale 1; **três colunas** (`u v peso`) formam um grafo com pesos (números reais, podendo ser negativos).
- Em grafo **direcionado**, a linha `u v` é a aresta **u → v** (e não v → u). Em grafo não direcionado, a aresta vale nos dois sentidos.

Exemplo (`exemplo.txt`):

```
5
1 2 2.0
2 3 -1.0
1 3 4.0
3 4 1.5
4 5 2.0
```

## Programas

### `graph_program` (demonstração)

```
./graph_program <arquivo> <matrix|list> [vertice_inicial] [--directed]
```

`vertice_inicial` (padrão 1) é a origem das buscas e o destino do Bellman-Ford. `--directed` lê o grafo como direcionado e pode aparecer em qualquer posição. Arquivos gerados na pasta atual:

| Arquivo | Conteúdo |
|---|---|
| `output_info.txt` | vértices, arestas, graus e componentes conexas |
| `output_bfs.txt` / `output_dfs.txt` | árvore de busca (pai e nível) a partir de `vertice_inicial` |
| `output_dijkstra.txt` | distância e pai de cada vértice (só se não houver pesos negativos) |
| `output_bellmanford.txt` | distância e próximo vértice no caminho de cada vértice até `vertice_inicial` (só se não houver ciclo negativo) |

Exemplo com o `exemplo.txt` acima:

```
./graph_program exemplo.txt list 5 --directed
cat output_bellmanford.txt
```

```
vertice distancia proximo
1 4.5 2
2 2.5 3
3 3.5 4
4 2 5
5 0 0
```

Lê-se: do vértice 1 até o 5 a distância é 4,5, e o caminho é 1 → 2 → 3 → 4 → 5.

### `case_study3` (Parte 3)

```
./case_study3 <grafo> [--directed] [--rep=list|matrix] [--runs=10] [--seed=42]
              [--skip-fw] [--fw-max-n=10000] [--bf-all-sample=K]
```

| Opção | Efeito |
|---|---|
| `--directed` | Lê o grafo como direcionado (padrão: não direcionado) |
| `--rep=list\|matrix` | Representação interna (padrão: `list`) |
| `--runs=N` | Número de rodadas das medições de tempo (padrão 10) |
| `--seed=N` | Semente do sorteio de vértices (padrão 42) |
| `--skip-fw` | Não executa o Floyd-Warshall |
| `--fw-max-n=N` | Só executa o Floyd-Warshall se n ≤ N (padrão 10000) |
| `--bf-all-sample=K` | Na comparação de todos os pares, roda o Bellman-Ford só para K destinos e **extrapola** para n destinos (resultado marcado como estimativa) |

Etapas executadas, nesta ordem:

1. **Bellman-Ford até o vértice 100:** distância dos vértices 10, 20 e 30 (e o caminho).
2. **Floyd-Warshall:** distâncias entre os vértices 10, 20, 30, 40 e 50, conferidas com o Bellman-Ford.
3. **Tempo médio** (em segundos, sem contar leitura do arquivo) de BFS, DFS, Dijkstra com heap, Bellman-Ford (1 destino) e Floyd-Warshall, com origens sorteadas.
4. **Bellman-Ford de todos os pares × Floyd-Warshall:** tempo do Bellman-Ford rodado para todos os destinos, comparado com o do Floyd-Warshall.

Comando usado para coletar os dados de um grafo grande:

```
./case_study3 grafo_W_1.txt --directed --runs=10 --skip-fw --bf-all-sample=5
```

> O Floyd-Warshall precisa de cerca de 12·(n+1)² bytes de memória (≈ 7 GiB para n = 25.000). Se a máquina não tiver essa memória livre, o sistema operacional encerra o processo (`Killed`); use `--skip-fw`.

### `case_study` (Parte 1) e `case_study2` (Parte 2)

Esses dois programas leem o grafo **sempre como não direcionado**.

```
./case_study  <grafo> [--rep=matrix|list|both] [--threshold=N]
./case_study2 <grafo> [--k=100] [--seed=42] [--rep=list|matrix] [--skip-vector] [--names=arquivo]
```

## Usando a biblioteca no seu código

```cpp
#include "Graph.hpp"
#include <iostream>

int main() {
    Graph g(Graph::Representation::LIST);
    g.loadFromFile("exemplo.txt", /*directed=*/true);

    // Bellman-Ford: distância de todos os vértices até o vértice 5
    Graph::BellmanFordResult bf = g.bellmanFord(5);
    if (!bf.negativeCycle) {
        std::cout << "dist(1 -> 5) = " << bf.dist[1] << "\n";
        for (int v : Graph::pathToTarget(bf, 1)) std::cout << v << " ";   // 1 2 3 4 5
    }

    // Floyd-Warshall: todos os pares
    Graph::AllPairsResult fw = g.floydWarshall();
    if (!fw.negativeCycle) {
        std::cout << "dist(2 -> 5) = " << fw.dist[2][5] << "\n";
        for (int v : Graph::pathBetween(fw, 2, 5)) std::cout << v << " "; // 2 3 4 5
    }
}
```

Compilação: `g++ -std=c++17 -O2 meu_programa.cpp Graph.cpp -o meu_programa`.

### Principais funções da classe `Graph`

| Função | Descrição |
|---|---|
| `Graph(rep)` | Cria um grafo com a representação `Representation::LIST` ou `Representation::MATRIX` |
| `loadFromFile(arquivo, directed = false)` | Lê o grafo do arquivo |
| `getNumVertices()`, `getNumEdges()`, `isDirected()`, `isWeighted()`, `hasNegativeWeights()` | Informações básicas |
| `degree(v)`, `minDegree()`, `maxDegree()`, `averageDegree()`, `medianDegree()` | Graus (grau de saída em grafos direcionados) |
| `writeGraphInfo(arquivo)` | Escreve informações gerais e componentes |
| `bfs(s)`, `dfs(s)` | Árvore de busca (`SearchTree`: `parent` e `level`) |
| `bfs(s, arquivo)`, `dfs(s, arquivo)` | Idem, gravando em arquivo |
| `distance(u, v)` | Distância em arestas (−1 se não há caminho) |
| `diameter()`, `diameterApprox()` | Diâmetro exato e aproximado |
| `connectedComponents()` | Componentes, da maior para a menor |
| `dijkstra(s, modo)` | Caminhos mínimos a partir de `s`; `modo` é `DijkstraMode::HEAP` (padrão) ou `VECTOR`; falha se houver pesos negativos |
| `weightedDistance(u, v)` | Distância ponderada (`Graph::INF` se não há caminho) |
| `Graph::pathTo(arvore, t)` | Caminho extraído de um resultado do Dijkstra |
| `bellmanFord(t)` | Distância e caminho de **todos** os vértices **até** `t`; informa ciclo negativo |
| `Graph::pathToTarget(resultado, v)` | Caminho de `v` até o destino, a partir do resultado do Bellman-Ford |
| `floydWarshall()` | Distâncias e árvores de caminhos entre todos os pares; informa ciclo negativo |
| `Graph::pathBetween(resultado, s, t)` | Caminho de `s` a `t`, a partir do resultado do Floyd-Warshall |

Resultados dos algoritmos de caminhos mínimos:

- **Dijkstra (`ShortestPathTree`):** `dist[v]` e `parent[v]` (0 na origem, −1 se inalcançável).
- **Bellman-Ford (`BellmanFordResult`):** `dist[v]` (distância de `v` até o destino), `next[v]` (próximo vértice do caminho até o destino; 0 no próprio destino, −1 se inalcançável), `negativeCycle` e `rounds` (número de rodadas executadas).
- **Floyd-Warshall (`AllPairsResult`):** `dist[s][t]`, `parent[s][t]` (pai de `t` na árvore de caminhos mínimos com raiz `s`, mesma convenção do Dijkstra) e `negativeCycle`.

## Decisões de projeto

- **Grafos direcionados.** Com `directed = true`, a linha `u v` cria só a aresta u → v. BFS, DFS e Dijkstra já percorriam as arestas de saída de cada vértice e funcionam sem alteração. O grau passa a ser o grau de saída, e as componentes conexas passam a ser as fracamente conexas (ignoram o sentido das arestas, calculadas com union-find).
- **Listas de arestas de entrada.** Em grafo direcionado com lista de adjacência, a biblioteca guarda também, para cada vértice `v`, quem aponta para ele (`revList`/`revWeights`). Isso custa O(m) de memória a mais e é o que o Bellman-Ford usa. Na matriz basta percorrer a coluna; em grafo não direcionado, as arestas de entrada são as de saída.
- **Bellman-Ford "todos → destino".** Parte do destino e anda para trás: quando a distância de `v` melhora, tenta melhorar a de cada `u` com aresta u → v. Duas otimizações: (1) para assim que uma rodada não altera nenhuma distância; (2) em cada rodada só processa os vértices cuja distância mudou na rodada anterior. Sem ciclo negativo, estabiliza em no máximo n−1 rodadas; se a rodada n ainda melhora alguma distância, há ciclo negativo.
- **Floyd-Warshall com uma única matriz.** Em vez de n+1 matrizes D⁽ᵏ⁾, usa uma matriz de distâncias atualizada no lugar (memória O(n²) em vez de O(n³)), mais uma matriz de pais para devolver as árvores de caminhos. As duas são matrizes (e não listas) porque a resposta já tem n² valores: o custo (n³ operações e 12·(n+1)² bytes) depende só de n, e uma lista só aumentaria a memória por par e o tempo de acesso.
- **Arestas paralelas.** A lista guarda todas; a matriz guarda só uma por par, e fica com o **menor** peso.

## Complexidade

| Algoritmo | Tempo | Memória extra |
|---|---|---|
| BFS / DFS | O(n + m) | O(n) |
| Dijkstra com heap | O((n + m) log n) | O(n) |
| Dijkstra com vetor | O(n²) | O(n) |
| Bellman-Ford (todos → 1 destino) | pior caso O(n·m); com as otimizações, O(R·m), onde R é o número de rodadas até estabilizar | O(n) |
| Bellman-Ford de todos os pares (n execuções) | pior caso O(n²·m) | O(n) |
| Floyd-Warshall | Θ(n³), independe de m | Θ(n²) (12·(n+1)² bytes) |

## Testes

`test_random.cpp` gera grafos aleatórios pequenos (direcionados e não direcionados, nas duas representações, com pesos positivos, negativos e ciclos negativos) e compara Bellman-Ford, Floyd-Warshall e Dijkstra, além de verificar se os caminhos retornados são coerentes. Para rodar (Linux ou macOS, pois usa `/tmp`):

```
g++ -std=c++17 -O1 -I. test_random.cpp Graph.cpp -o test_random
./test_random
```

A saída termina com a contagem de casos e de erros (`erros=0` quando tudo concorda).

## Limitações

- **Memória do Floyd-Warshall:** cresce com n², então só é viável para grafos de poucos milhares de vértices (≈ 7 GiB já para n = 25.000).
- **Ciclo negativo no Bellman-Ford:** é detectado apenas quando o ciclo alcança o vértice de destino. O Floyd-Warshall detecta ciclos negativos em qualquer parte do grafo.
- **Grafo não direcionado com aresta negativa:** forma ciclo negativo (ida e volta pela mesma aresta). Para pesos negativos, leia o grafo como direcionado.
- **Diâmetro aproximado em grafo direcionado:** a heurística de duplo alcance não tem garantia nesse caso; o valor devolvido é um limite inferior do diâmetro.
- **`MemoryUtils.hpp`** só funciona em Linux (devolve 0 em outros sistemas).
- **`case_study` e `case_study2`** não aceitam `--directed`.
