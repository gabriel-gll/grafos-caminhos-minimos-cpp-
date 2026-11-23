#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <limits>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Graph
//
// Biblioteca para manipulacao de grafos (nao-direcionados OU direcionados),
// com ou sem pesos nas arestas. O usuario escolhe se o grafo e direcionado
// ao chamar loadFromFile(arquivo, directed). Em grafos direcionados, a aresta
// da linha "u v [peso]" do arquivo vai de u para v.
// Suporta duas representacoes internas (o usuario escolhe qual usar):
//   - MATRIX : matriz de adjacencia (vector<vector<int>>, 0/1)
//   - LIST   : lista de adjacencia (vector<vector<int>>, vizinhos de cada v.)
//
// Vertices sao numerados de 1 a n (o vertice 0 nao e usado, apenas para
// simplificar a indexacao dos vetores).
// ---------------------------------------------------------------------------
class Graph {
public:
    enum class Representation { MATRIX, LIST };

    // Estrutura de retorno para BFS/DFS: para cada vertice, guarda o pai na
    // arvore de busca (0 se for a raiz, -1 se o vertice nao foi visitado) e o
    // nivel na arvore (-1 se nao visitado).
    struct SearchTree {
        std::vector<int> parent;
        std::vector<int> level;
    };

    // Resultado da busca de componentes conexas: cada componente e uma lista
    // de vertices. O vetor externo vem ordenado em ordem decrescente de
    // tamanho (maior componente primeiro).
    using Components = std::vector<std::vector<int>>;

    // ---- Parte 2: caminhos minimos -----------------------------------------
    // Resultado de Dijkstra: dist[v] = distancia minima da origem a v
    // (INFINITY se inalcancavel); parent[v] = pai de v na arvore de caminhos
    // minimos (0 para a origem, -1 se inalcancavel).
    struct ShortestPathTree {
        int source = 0;
        std::vector<double> dist;
        std::vector<int> parent;
    };

    // Como as estimativas de distancia sao armazenadas / o minimo e extraido:
    //   VECTOR : vetor simples, extracao do minimo por varredura -- O(n^2)
    //   HEAP   : heap binario indexado com decrease-key        -- O((n+m) log n)
    enum class DijkstraMode { VECTOR, HEAP };

    static constexpr double INF = std::numeric_limits<double>::infinity();

    // ---- Parte 3: Bellman-Ford e Floyd-Warshall ----------------------------
    // Resultado do Bellman-Ford "todos -> target":
    //   dist[v] = distancia minima de v ate target (INF se v nao alcanca target)
    //   next[v] = proximo vertice no caminho minimo de v ate target
    //             (0 para o proprio target, -1 se inalcancavel). Ou seja, a
    //             arvore de caminhos minimos e uma arvore enraizada em target,
    //             com as arestas apontando para a raiz.
    //   negativeCycle = true se ha ciclo negativo que alcanca target; nesse
    //             caso dist/next NAO sao confiaveis.
    struct BellmanFordResult {
        int target = 0;
        std::vector<double> dist;
        std::vector<int> next;
        bool negativeCycle = false;
        int rounds = 0; // numero de iteracoes (rodadas) efetivamente executadas
    };

    // Resultado do Floyd-Warshall (todos os pares). Indices de 1 a n.
    //   dist[s][t]   = distancia minima de s a t (INF se inalcancavel)
    //   parent[s][t] = pai de t na arvore de caminhos minimos com raiz s
    //                  (0 se t == s, -1 se inalcancavel) -- mesma convencao de
    //                  ShortestPathTree. A linha parent[s] e a arvore de s.
    //   negativeCycle = true se o grafo tem ciclo negativo (dist/parent
    //                  nao sao confiaveis).
    struct AllPairsResult {
        bool negativeCycle = false;
        std::vector<std::vector<double>> dist;
        std::vector<std::vector<int>> parent;
    };

    explicit Graph(Representation rep = Representation::LIST);

    // ---- Construcao / entrada -------------------------------------------
    // Le o grafo a partir de um arquivo texto no formato descrito no
    // enunciado: primeira linha = numero de vertices; linhas seguintes = uma
    // aresta "u v" por linha.
    // "directed" indica se o grafo e direcionado (padrao: nao). Em grafos
    // direcionados, degree(v) e o grau de SAIDA e connectedComponents()
    // devolve as componentes FRACAMENTE conexas (ignora o sentido das arestas).
    void loadFromFile(const std::string& filename, bool directed = false);
    bool isDirected() const { return directed; }

    // ---- Informacoes basicas ---------------------------------------------
    // O arquivo pode ter 2 colunas ("u v", grafo sem pesos; cada aresta vale
    // 1) ou 3 colunas ("u v peso"). O formato e decidido pela primeira aresta.
    bool isWeighted() const { return weighted; }
    bool hasNegativeWeights() const { return negativeWeights; }
    int getNumVertices() const { return numVertices; }
    int getNumEdges() const { return numEdges; }
    Representation getRepresentation() const { return representation; }

    int degree(int v) const;
    int minDegree() const;
    int maxDegree() const;
    double averageDegree() const;
    double medianDegree() const;

    // Escreve em um arquivo texto: numero de vertices, numero de arestas,
    // grau minimo, maximo, medio, mediana e informacoes das componentes
    // conexas (quantidade, tamanho de cada uma e vertices pertencentes).
    void writeGraphInfo(const std::string& filename) const;

    // ---- Buscas -------------------------------------------------------
    // Executa BFS/DFS a partir de "start" e grava a arvore de busca
    // (pai e nivel de cada vertice) no arquivo indicado.
    SearchTree bfs(int start) const;
    SearchTree dfs(int start) const;

    void bfs(int start, const std::string& outFilename) const;
    void dfs(int start, const std::string& outFilename) const;

    // ---- Distancias e diametro --------------------------------------------
    // Distancia (numero de arestas do caminho minimo) entre u e v.
    // Retorna -1 se nao houver caminho entre eles.
    int distance(int u, int v) const;

    // Diametro exato: maior distancia entre qualquer par de vertices.
    // Custo: O(n * (n + m)) -- BFS a partir de cada vertice.
    int diameter() const;

    // Diametro aproximado (heuristica de "double sweep"): faz uma BFS a
    // partir de um vertice arbitrario para achar o vertice mais distante a;
    // faz uma segunda BFS a partir de a para achar o vertice mais distante b;
    // retorna dist(a, b) como aproximacao do diametro. Custo: O(n + m).
    int diameterApprox() const;

    // ---- Componentes conexas ----------------------------------------------
    Components connectedComponents() const;

    // ---- Caminhos minimos (Dijkstra) --------------------------------------
    // Distancia e arvore de caminhos minimos de "start" para todos os
    // vertices. Lanca std::runtime_error se o grafo tiver pesos negativos
    // (Dijkstra nao se aplica; use bellmanFord() ou floydWarshall()).
    // Ambos os modos usam o MESMO codigo de Dijkstra; so muda a estrutura
    // que guarda as estimativas e extrai o minimo.
    ShortestPathTree dijkstra(int start, DijkstraMode mode = DijkstraMode::HEAP) const;

    // Grava "vertice distancia pai" para todos os vertices.
    void dijkstra(int start, const std::string& outFilename,
                  DijkstraMode mode = DijkstraMode::HEAP) const;

    // Distancia ponderada de u a v (INF se nao houver caminho).
    double weightedDistance(int u, int v, DijkstraMode mode = DijkstraMode::HEAP) const;

    // Caminho minimo origem -> target extraido da arvore (vazio se inalcancavel).
    static std::vector<int> pathTo(const ShortestPathTree& tree, int target);

    // ---- Parte 3 ------------------------------------------------------------
    // Bellman-Ford: distancia e caminho minimo de TODOS os vertices ate
    // "target". Aceita pesos negativos e detecta ciclo negativo. Duas
    // otimizacoes: (1) para assim que uma rodada nao altera nada; (2) em cada
    // rodada so processa os vertices cuja distancia mudou na rodada anterior.
    BellmanFordResult bellmanFord(int target) const;
    // Grava "vertice distancia proximo" para todos os vertices.
    void bellmanFord(int target, const std::string& outFilename) const;
    // Caminho minimo v -> target (vazio se inalcancavel).
    static std::vector<int> pathToTarget(const BellmanFordResult& r, int v);

    // Floyd-Warshall: distancias e arvores de caminhos minimos entre todos os
    // pares, com UMA unica matriz de distancias atualizada in-place (memoria
    // O(n^2) em vez de O(n^3) com as matrizes D^(k)).
    AllPairsResult floydWarshall() const;
    // Caminho minimo s -> t (vazio se inalcancavel).
    static std::vector<int> pathBetween(const AllPairsResult& r, int s, int t);

private:
    int numVertices;
    int numEdges;
    Representation representation;

    std::vector<std::vector<int>> adjMatrix; // usado se representation == MATRIX
    std::vector<std::vector<int>> adjList;   // usado se representation == LIST

    // Pesos (so alocados se o grafo for com pesos):
    //   MATRIX: weightMatrix[u][v] vale o peso se adjMatrix[u][v] == 1
    //   LIST  : adjWeights[u][i] e o peso da aresta para adjList[u][i]
    // (adjMatrix/adjList continuam intactos, entao a Parte 1 nao muda.)
    std::vector<std::vector<double>> weightMatrix;
    std::vector<std::vector<double>> adjWeights;
    bool weighted = false;
    bool negativeWeights = false;
    bool directed = false;

    // Arestas de ENTRADA (so alocadas em grafos direcionados com LIST):
    // revList[v] = vertices u com aresta u->v; revWeights[v][i] = peso dela.
    // Necessarias para o Bellman-Ford "todos -> target" (relaxa arestas de
    // entrada). Na MATRIX basta percorrer a coluna; em grafos nao direcionados
    // as arestas de entrada sao as de saida.
    std::vector<std::vector<int>> revList;
    std::vector<std::vector<double>> revWeights;

    void addEdge(int u, int v, double w = 1.0);
    std::vector<int> neighbors(int v) const;

    // BFS auxiliar usada por distance()/diameter(); retorna o vetor de
    // niveis (distancias) a partir de "start" (-1 para vertices nao
    // alcancados).
    std::vector<int> bfsLevels(int start) const;

    // Chama fn(vizinho, peso) para cada aresta de v, sem copiar vizinhos.
    template <typename Fn>
    void forEachWeightedNeighbor(int v, Fn fn) const;

    // Chama fn(u, peso) para cada aresta u->v que ENTRA em v.
    template <typename Fn>
    void forEachInNeighbor(int v, Fn fn) const;

    template <typename PQ>
    ShortestPathTree runDijkstra(int start) const;
};

#endif // GRAPH_HPP
