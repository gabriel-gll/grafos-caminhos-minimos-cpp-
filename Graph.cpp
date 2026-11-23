#include "Graph.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <cmath>
#include <numeric>
#include <sstream>
#include <stdexcept>

Graph::Graph(Representation rep)
    : numVertices(0), numEdges(0), representation(rep) {}

// ---------------------------------------------------------------------------
// Entrada
// ---------------------------------------------------------------------------
void Graph::loadFromFile(const std::string& filename, bool isDirected) {
    directed = isDirected;
    std::ifstream in(filename);
    if (!in.is_open()) {
        throw std::runtime_error("Nao foi possivel abrir o arquivo: " + filename);
    }

    std::string line;

    // Primeira linha: numero de vertices.
    if (!std::getline(in, line)) {
        throw std::runtime_error("Arquivo de entrada vazio: " + filename);
    }
    {
        std::istringstream iss(line);
        if (!(iss >> numVertices)) {
            throw std::runtime_error("Formato invalido na primeira linha do arquivo.");
        }
    }

    numEdges = 0;
    weighted = false;
    negativeWeights = false;
    weightMatrix.clear();
    adjWeights.clear();
    revList.clear();
    revWeights.clear();

    if (representation == Representation::MATRIX) {
        adjMatrix.assign(numVertices + 1, std::vector<int>(numVertices + 1, 0));
    } else {
        adjList.assign(numVertices + 1, std::vector<int>());
        if (directed) revList.assign(numVertices + 1, std::vector<int>());
    }

    // O formato (com/sem pesos) e decidido pela primeira aresta do arquivo:
    // so entao alocamos as estruturas de pesos, para que grafos sem pesos
    // nao paguem memoria extra.
    bool formatDecided = false;

    // Linhas seguintes: arestas "u v".
    while (std::getline(in, line)) {
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue; // ignora linhas em branco
        }
        std::istringstream iss(line);
        int u, v;
        if (!(iss >> u >> v)) {
            continue; // ignora linhas malformadas
        }
        double w = 1.0;
        bool hasWeight = static_cast<bool>(iss >> w);

        if (!formatDecided) {
            formatDecided = true;
            weighted = hasWeight;
            if (weighted) {
                if (representation == Representation::MATRIX) {
                    weightMatrix.assign(numVertices + 1,
                                        std::vector<double>(numVertices + 1, 0.0));
                } else {
                    adjWeights.assign(numVertices + 1, std::vector<double>());
                    if (directed) {
                        revWeights.assign(numVertices + 1, std::vector<double>());
                    }
                }
            }
        } else if (weighted && !hasWeight) {
            throw std::runtime_error("Aresta sem peso em arquivo com pesos: " + line);
        }
        if (!weighted) w = 1.0;

        addEdge(u, v, w);
    }

    in.close();
}

void Graph::addEdge(int u, int v, double w) {
    if (u < 1 || u > numVertices || v < 1 || v > numVertices) {
        throw std::runtime_error("Aresta referencia vertice fora do intervalo [1, n].");
    }

    if (representation == Representation::MATRIX) {
        // Arestas paralelas (mesmo par no arquivo): a matriz so guarda um
        // peso por par, entao mantemos o MENOR -- igual ao efeito que as
        // arestas paralelas da LIST tem nos caminhos minimos.
        if (weighted && (!adjMatrix[u][v] || w < weightMatrix[u][v])) {
            weightMatrix[u][v] = w;
        }
        adjMatrix[u][v] = 1;
        if (!directed) {
            if (weighted && (!adjMatrix[v][u] || w < weightMatrix[v][u])) {
                weightMatrix[v][u] = w;
            }
            adjMatrix[v][u] = 1;
        }
    } else {
        adjList[u].push_back(v);
        if (weighted) adjWeights[u].push_back(w);
        if (directed) {
            // guarda tambem a aresta de entrada (v <- u)
            revList[v].push_back(u);
            if (weighted) revWeights[v].push_back(w);
        } else {
            adjList[v].push_back(u);
            if (weighted) adjWeights[v].push_back(w);
        }
    }
    if (w < 0) negativeWeights = true;

    ++numEdges;
}

std::vector<int> Graph::neighbors(int v) const {
    std::vector<int> result;
    if (representation == Representation::MATRIX) {
        result.reserve(numVertices);
        for (int u = 1; u <= numVertices; ++u) {
            if (adjMatrix[v][u]) {
                result.push_back(u);
            }
        }
    } else {
        result = adjList[v];
    }
    return result;
}

// ---------------------------------------------------------------------------
// Estatisticas de grau
// ---------------------------------------------------------------------------
int Graph::degree(int v) const {
    if (representation == Representation::MATRIX) {
        int d = 0;
        for (int u = 1; u <= numVertices; ++u) {
            d += adjMatrix[v][u];
        }
        return d;
    }
    return static_cast<int>(adjList[v].size());
}

int Graph::minDegree() const {
    int best = numVertices > 0 ? degree(1) : 0;
    for (int v = 2; v <= numVertices; ++v) {
        best = std::min(best, degree(v));
    }
    return best;
}

int Graph::maxDegree() const {
    int best = numVertices > 0 ? degree(1) : 0;
    for (int v = 2; v <= numVertices; ++v) {
        best = std::max(best, degree(v));
    }
    return best;
}

double Graph::averageDegree() const {
    if (numVertices == 0) return 0.0;
    long long sum = 0;
    for (int v = 1; v <= numVertices; ++v) {
        sum += degree(v);
    }
    return static_cast<double>(sum) / numVertices;
}

double Graph::medianDegree() const {
    if (numVertices == 0) return 0.0;
    std::vector<int> degrees(numVertices);
    for (int v = 1; v <= numVertices; ++v) {
        degrees[v - 1] = degree(v);
    }
    std::sort(degrees.begin(), degrees.end());

    size_t n = degrees.size();
    if (n % 2 == 1) {
        return degrees[n / 2];
    }
    return (degrees[n / 2 - 1] + degrees[n / 2]) / 2.0;
}

// ---------------------------------------------------------------------------
// BFS / DFS
// ---------------------------------------------------------------------------
Graph::SearchTree Graph::bfs(int start) const {
    SearchTree tree;
    tree.parent.assign(numVertices + 1, -1);
    tree.level.assign(numVertices + 1, -1);

    if (start < 1 || start > numVertices) {
        throw std::runtime_error("Vertice inicial fora do intervalo [1, n].");
    }

    std::vector<char> visited(numVertices + 1, 0);
    std::vector<int> queue;
    queue.reserve(numVertices);

    size_t head = 0;
    visited[start] = 1;
    tree.parent[start] = 0; // raiz nao tem pai; convencao: 0
    tree.level[start] = 0;
    queue.push_back(start);

    while (head < queue.size()) {
        int u = queue[head++];
        for (int w : neighbors(u)) {
            if (!visited[w]) {
                visited[w] = 1;
                tree.parent[w] = u;
                tree.level[w] = tree.level[u] + 1;
                queue.push_back(w);
            }
        }
    }

    return tree;
}

Graph::SearchTree Graph::dfs(int start) const {
    SearchTree tree;
    tree.parent.assign(numVertices + 1, -1);
    tree.level.assign(numVertices + 1, -1);

    if (start < 1 || start > numVertices) {
        throw std::runtime_error("Vertice inicial fora do intervalo [1, n].");
    }

    std::vector<char> visited(numVertices + 1, 0);

    // Pilha explicita para evitar estouro de pilha em grafos grandes.
    // Cada item: (vertice, pai, nivel)
    struct Frame { int v, parent, level; };
    std::vector<Frame> stack;
    stack.push_back({start, 0, 0});

    while (!stack.empty()) {
        Frame f = stack.back();
        stack.pop_back();

        if (visited[f.v]) continue;

        visited[f.v] = 1;
        tree.parent[f.v] = f.parent;
        tree.level[f.v] = f.level;

        // Empilha vizinhos em ordem reversa para manter uma ordem de
        // visita "natural" (menor vizinho explorado primeiro).
        std::vector<int> adj = neighbors(f.v);
        for (auto it = adj.rbegin(); it != adj.rend(); ++it) {
            if (!visited[*it]) {
                stack.push_back({*it, f.v, f.level + 1});
            }
        }
    }

    return tree;
}

static void writeSearchTreeToFile(const Graph::SearchTree& tree,
                                   int numVertices,
                                   const std::string& outFilename) {
    std::ofstream out(outFilename);
    if (!out.is_open()) {
        throw std::runtime_error("Nao foi possivel criar o arquivo: " + outFilename);
    }
    out << "vertice pai nivel\n";
    for (int v = 1; v <= numVertices; ++v) {
        out << v << " " << tree.parent[v] << " " << tree.level[v] << "\n";
    }
    out.close();
}

void Graph::bfs(int start, const std::string& outFilename) const {
    SearchTree tree = bfs(start);
    writeSearchTreeToFile(tree, numVertices, outFilename);
}

void Graph::dfs(int start, const std::string& outFilename) const {
    SearchTree tree = dfs(start);
    writeSearchTreeToFile(tree, numVertices, outFilename);
}

// ---------------------------------------------------------------------------
// Distancias e diametro
// ---------------------------------------------------------------------------
std::vector<int> Graph::bfsLevels(int start) const {
    return bfs(start).level;
}

int Graph::distance(int u, int v) const {
    if (u < 1 || u > numVertices || v < 1 || v > numVertices) {
        throw std::runtime_error("Vertice fora do intervalo [1, n].");
    }
    std::vector<int> levels = bfsLevels(u);
    return levels[v]; // -1 se nao alcancavel
}

int Graph::diameter() const {
    int best = 0;
    for (int v = 1; v <= numVertices; ++v) {
        std::vector<int> levels = bfsLevels(v);
        for (int u = 1; u <= numVertices; ++u) {
            if (levels[u] > best) {
                best = levels[u];
            }
        }
    }
    return best;
}

int Graph::diameterApprox() const {
    if (numVertices == 0) return 0;

    // 1a BFS a partir de um vertice arbitrario (vertice 1).
    std::vector<int> levels1 = bfsLevels(1);
    int a = 1, bestDist = 0;
    for (int v = 1; v <= numVertices; ++v) {
        if (levels1[v] > bestDist) {
            bestDist = levels1[v];
            a = v;
        }
    }

    // 2a BFS a partir do vertice mais distante encontrado.
    std::vector<int> levels2 = bfsLevels(a);
    int approx = 0;
    for (int v = 1; v <= numVertices; ++v) {
        if (levels2[v] > approx) {
            approx = levels2[v];
        }
    }

    // Em grafos direcionados o double sweep nao e garantido (a 2a BFS pode
    // partir de um vertice sem arestas de saida); devolvemos o maior valor
    // visto nas duas varreduras, que e sempre um limite inferior do diametro.
    return std::max(approx, bestDist);
}

// ---------------------------------------------------------------------------
// Componentes conexas
// ---------------------------------------------------------------------------
Graph::Components Graph::connectedComponents() const {
    Components components;

    if (directed) {
        // Componentes FRACAMENTE conexas: ignora o sentido das arestas.
        // A BFS abaixo so segue arestas de saida, entao usamos union-find.
        std::vector<int> root(numVertices + 1);
        std::iota(root.begin(), root.end(), 0);
        auto find = [&](int x) {
            while (root[x] != x) { root[x] = root[root[x]]; x = root[x]; }
            return x;
        };
        for (int u = 1; u <= numVertices; ++u) {
            forEachWeightedNeighbor(u, [&](int w, double) {
                int a = find(u), b = find(w);
                if (a != b) root[a] = b;
            });
        }
        std::vector<std::vector<int>> groups(numVertices + 1);
        for (int v = 1; v <= numVertices; ++v) groups[find(v)].push_back(v);
        for (auto& g : groups) {
            if (!g.empty()) components.push_back(std::move(g));
        }
        std::sort(components.begin(), components.end(),
                  [](const std::vector<int>& a, const std::vector<int>& b) {
                      return a.size() > b.size();
                  });
        return components;
    }

    std::vector<char> visited(numVertices + 1, 0);

    for (int start = 1; start <= numVertices; ++start) {
        if (visited[start]) continue;

        std::vector<int> component;
        std::vector<int> queue;
        size_t head = 0;

        visited[start] = 1;
        queue.push_back(start);

        while (head < queue.size()) {
            int u = queue[head++];
            component.push_back(u);
            for (int w : neighbors(u)) {
                if (!visited[w]) {
                    visited[w] = 1;
                    queue.push_back(w);
                }
            }
        }

        std::sort(component.begin(), component.end());
        components.push_back(std::move(component));
    }

    std::sort(components.begin(), components.end(),
              [](const std::vector<int>& a, const std::vector<int>& b) {
                  return a.size() > b.size();
              });

    return components;
}

// ---------------------------------------------------------------------------
// Saida: informacoes gerais do grafo
// ---------------------------------------------------------------------------
void Graph::writeGraphInfo(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        throw std::runtime_error("Nao foi possivel criar o arquivo: " + filename);
    }

    out << "numero de vertices: " << numVertices << "\n";
    out << "numero de arestas: " << numEdges << "\n";
    out << "grau minimo: " << minDegree() << "\n";
    out << "grau maximo: " << maxDegree() << "\n";
    out << "grau medio: " << averageDegree() << "\n";
    out << "mediana de grau: " << medianDegree() << "\n";
    out << "\n";

    Components components = connectedComponents();
    out << "numero de componentes conexas: " << components.size() << "\n";
    for (size_t i = 0; i < components.size(); ++i) {
        out << "componente " << (i + 1) << " (tamanho " << components[i].size() << "): ";
        for (size_t j = 0; j < components[i].size(); ++j) {
            out << components[i][j];
            if (j + 1 < components[i].size()) out << " ";
        }
        out << "\n";
    }

    out.close();
}

// ---------------------------------------------------------------------------
// Parte 2: Dijkstra
// ---------------------------------------------------------------------------

template <typename Fn>
void Graph::forEachWeightedNeighbor(int v, Fn fn) const {
    if (representation == Representation::MATRIX) {
        for (int u = 1; u <= numVertices; ++u) {
            if (adjMatrix[v][u]) {
                fn(u, weighted ? weightMatrix[v][u] : 1.0);
            }
        }
    } else {
        const std::vector<int>& adj = adjList[v];
        for (size_t i = 0; i < adj.size(); ++i) {
            fn(adj[i], weighted ? adjWeights[v][i] : 1.0);
        }
    }
}

// As duas estruturas abaixo expoem a MESMA interface, usada por runDijkstra:
//   update(v, key) : define/diminui a estimativa de v (insere se necessario)
//   extractMin()   : remove e devolve o vertice de menor estimativa (-1 se vazio)
namespace {

// (1) Vetor: update e O(1); extractMin varre todos os vertices -- O(n).
class VectorQueue {
public:
    explicit VectorQueue(int n) : key(n + 1, Graph::INF), active(n + 1, 0) {}
    void update(int v, double k) { key[v] = k; active[v] = 1; }
    int extractMin() {
        int best = -1;
        for (size_t v = 1; v < key.size(); ++v) {
            if (active[v] && (best == -1 || key[v] < key[best])) {
                best = static_cast<int>(v);
            }
        }
        if (best != -1) active[best] = 0;
        return best;
    }
private:
    std::vector<double> key;
    std::vector<char> active;
};

// (2) Heap binario indexado: pos[v] guarda onde v esta no heap, o que
// permite diminuir a chave de um elemento existente em O(log n)
// (decrease-key), sem duplicar entradas como faria um std::priority_queue.
class HeapQueue {
public:
    explicit HeapQueue(int n) : key(n + 1, Graph::INF), pos(n + 1, -1) {
        heap.reserve(n);
    }
    void update(int v, double k) {
        key[v] = k;
        if (pos[v] == -1) {
            pos[v] = static_cast<int>(heap.size());
            heap.push_back(v);
        }
        siftUp(pos[v]);
    }
    int extractMin() {
        if (heap.empty()) return -1;
        int top = heap[0];
        int last = heap.back();
        heap.pop_back();
        pos[top] = -1;
        if (!heap.empty()) {
            heap[0] = last;
            pos[last] = 0;
            siftDown(0);
        }
        return top;
    }
private:
    std::vector<double> key;
    std::vector<int> pos;
    std::vector<int> heap;

    void place(int i, int v) { heap[i] = v; pos[v] = i; }
    void siftUp(int i) {
        int v = heap[i];
        while (i > 0) {
            int p = (i - 1) / 2;
            if (key[heap[p]] <= key[v]) break;
            place(i, heap[p]);
            i = p;
        }
        place(i, v);
    }
    void siftDown(int i) {
        int v = heap[i];
        int n = static_cast<int>(heap.size());
        while (true) {
            int c = 2 * i + 1;
            if (c >= n) break;
            if (c + 1 < n && key[heap[c + 1]] < key[heap[c]]) ++c;
            if (key[v] <= key[heap[c]]) break;
            place(i, heap[c]);
            i = c;
        }
        place(i, v);
    }
};

} // namespace

// Unico codigo de Dijkstra; PQ define como as estimativas sao armazenadas.
template <typename PQ>
Graph::ShortestPathTree Graph::runDijkstra(int start) const {
    ShortestPathTree tree;
    tree.source = start;
    tree.dist.assign(numVertices + 1, INF);
    tree.parent.assign(numVertices + 1, -1);

    PQ pq(numVertices);
    tree.dist[start] = 0.0;
    tree.parent[start] = 0;
    pq.update(start, 0.0);

    int u;
    while ((u = pq.extractMin()) != -1) {
        double du = tree.dist[u];
        forEachWeightedNeighbor(u, [&](int w, double weight) {
            double nd = du + weight;
            if (nd < tree.dist[w]) {
                tree.dist[w] = nd;
                tree.parent[w] = u;
                pq.update(w, nd);
            }
        });
    }
    return tree;
}

Graph::ShortestPathTree Graph::dijkstra(int start, DijkstraMode mode) const {
    if (start < 1 || start > numVertices) {
        throw std::runtime_error("Vertice inicial fora do intervalo [1, n].");
    }
    if (negativeWeights) {
        throw std::runtime_error(
            "O grafo possui pesos negativos: Dijkstra nao se aplica. "
            "Use bellmanFord() ou floydWarshall().");
    }
    if (mode == DijkstraMode::VECTOR) return runDijkstra<VectorQueue>(start);
    return runDijkstra<HeapQueue>(start);
}

void Graph::dijkstra(int start, const std::string& outFilename, DijkstraMode mode) const {
    ShortestPathTree tree = dijkstra(start, mode);
    std::ofstream out(outFilename);
    if (!out.is_open()) {
        throw std::runtime_error("Nao foi possivel criar o arquivo: " + outFilename);
    }
    out << "vertice distancia pai\n";
    for (int v = 1; v <= numVertices; ++v) {
        out << v << " ";
        if (std::isinf(tree.dist[v])) out << "inf";
        else out << tree.dist[v];
        out << " " << tree.parent[v] << "\n";
    }
}

double Graph::weightedDistance(int u, int v, DijkstraMode mode) const {
    if (v < 1 || v > numVertices) {
        throw std::runtime_error("Vertice fora do intervalo [1, n].");
    }
    return dijkstra(u, mode).dist[v];
}

std::vector<int> Graph::pathTo(const ShortestPathTree& tree, int target) {
    std::vector<int> path;
    if (target < 1 || target >= static_cast<int>(tree.parent.size()) ||
        tree.parent[target] == -1) {
        return path;
    }
    for (int v = target; v != 0; v = tree.parent[v]) path.push_back(v);
    std::reverse(path.begin(), path.end());
    return path;
}

// ---------------------------------------------------------------------------
// Parte 3: Bellman-Ford e Floyd-Warshall
// ---------------------------------------------------------------------------

template <typename Fn>
void Graph::forEachInNeighbor(int v, Fn fn) const {
    if (!directed) {
        // Nao direcionado: arestas de entrada == arestas de saida.
        forEachWeightedNeighbor(v, fn);
    } else if (representation == Representation::MATRIX) {
        for (int u = 1; u <= numVertices; ++u) {
            if (adjMatrix[u][v]) fn(u, weighted ? weightMatrix[u][v] : 1.0);
        }
    } else {
        const std::vector<int>& in = revList[v];
        for (size_t i = 0; i < in.size(); ++i) {
            fn(in[i], weighted ? revWeights[v][i] : 1.0);
        }
    }
}

// Bellman-Ford "todos -> target".
//
// Como queremos a distancia de cada vertice ATE target, a propagacao parte de
// target e anda "para tras": quando dist[v] melhora, tentamos melhorar
// dist[u] para toda aresta u->v (dist[u] = w(u,v) + dist[v]).
//
// Otimizacoes:
//  (1) Parada antecipada: se uma rodada nao altera nenhuma distancia, o
//      algoritmo termina (nao ha por que fazer as n-1 rodadas).
//  (2) Apenas vertices "ativos": na rodada k so relaxamos as arestas de
//      entrada dos vertices cuja distancia mudou na rodada k-1 -- os demais
//      nao podem gerar melhoria nova. (A lista de ativos vazia equivale a (1).)
//
// Ciclo negativo: sem ciclo negativo, as distancias estabilizam em no maximo
// n-1 rodadas; se a rodada n ainda altera alguma distancia, ha ciclo negativo
// (que alcanca target).
Graph::BellmanFordResult Graph::bellmanFord(int target) const {
    if (target < 1 || target > numVertices) {
        throw std::runtime_error("Vertice alvo fora do intervalo [1, n].");
    }

    BellmanFordResult r;
    r.target = target;
    r.dist.assign(numVertices + 1, INF);
    r.next.assign(numVertices + 1, -1);
    r.dist[target] = 0.0;
    r.next[target] = 0;

    std::vector<int> active{target};
    std::vector<int> updated;
    std::vector<char> inUpdated(numVertices + 1, 0);

    while (!active.empty()) {
        ++r.rounds;
        updated.clear();

        for (int v : active) {
            double dv = r.dist[v];
            forEachInNeighbor(v, [&](int u, double w) {
                double nd = dv + w;
                if (nd < r.dist[u]) {
                    r.dist[u] = nd;
                    r.next[u] = v;
                    if (!inUpdated[u]) {
                        inUpdated[u] = 1;
                        updated.push_back(u);
                    }
                }
            });
        }

        for (int u : updated) inUpdated[u] = 0;

        if (r.rounds >= numVertices && !updated.empty()) {
            r.negativeCycle = true; // ainda melhora na rodada n
            break;
        }
        active.swap(updated);
    }
    return r;
}

void Graph::bellmanFord(int target, const std::string& outFilename) const {
    BellmanFordResult r = bellmanFord(target);
    std::ofstream out(outFilename);
    if (!out.is_open()) {
        throw std::runtime_error("Nao foi possivel criar o arquivo: " + outFilename);
    }
    if (r.negativeCycle) {
        out << "ciclo negativo detectado\n";
        return;
    }
    out << "vertice distancia proximo\n";
    for (int v = 1; v <= numVertices; ++v) {
        out << v << " ";
        if (std::isinf(r.dist[v])) out << "inf";
        else out << r.dist[v];
        out << " " << r.next[v] << "\n";
    }
}

std::vector<int> Graph::pathToTarget(const BellmanFordResult& r, int v) {
    std::vector<int> path;
    if (v < 1 || v >= static_cast<int>(r.next.size()) || r.next[v] == -1) {
        return path;
    }
    for (int x = v; x != 0; x = r.next[x]) {
        path.push_back(x);
        if (path.size() > r.next.size()) return {}; // ciclo (resultado invalido)
    }
    return path;
}

// Floyd-Warshall com memoria O(n^2).
//
// A formulacao classica usa D^(k) para k = 0..n (n+1 matrizes). Como a linha k
// e a coluna k nao mudam durante a iteracao k (d[k][k] >= 0 enquanto nao ha
// ciclo negativo), basta UMA matriz atualizada in-place. A matriz parent
// guarda a arvore de caminhos minimos de cada origem: parent[i][j] = penultimo
// vertice do caminho minimo i -> j; ao melhorar i -> j passando por k,
// parent[i][j] = parent[k][j].
Graph::AllPairsResult Graph::floydWarshall() const {
    const int n = numVertices;
    AllPairsResult r;
    r.dist.assign(n + 1, std::vector<double>(n + 1, INF));
    r.parent.assign(n + 1, std::vector<int>(n + 1, -1));

    for (int i = 1; i <= n; ++i) {
        r.dist[i][i] = 0.0;
        r.parent[i][i] = 0;
    }
    for (int u = 1; u <= n; ++u) {
        forEachWeightedNeighbor(u, [&](int v, double w) {
            if (u == v) {
                // laco: so importa se for negativo (ciclo negativo)
                if (w < r.dist[u][u]) r.dist[u][u] = w;
            } else if (w < r.dist[u][v]) { // arestas paralelas: fica a menor
                r.dist[u][v] = w;
                r.parent[u][v] = u;
            }
        });
    }

    for (int k = 1; k <= n; ++k) {
        const std::vector<double>& dk = r.dist[k];
        const std::vector<int>& pk = r.parent[k];
        for (int i = 1; i <= n; ++i) {
            double dik = r.dist[i][k];
            if (dik == INF) continue; // i nao chega a k
            std::vector<double>& di = r.dist[i];
            std::vector<int>& pi = r.parent[i];
            for (int j = 1; j <= n; ++j) {
                double nd = dik + dk[j];
                if (nd < di[j]) {
                    di[j] = nd;
                    pi[j] = pk[j];
                }
            }
        }
    }

    for (int i = 1; i <= n; ++i) {
        if (r.dist[i][i] < 0) { r.negativeCycle = true; break; }
    }
    return r;
}

std::vector<int> Graph::pathBetween(const AllPairsResult& r, int s, int t) {
    std::vector<int> path;
    int n = static_cast<int>(r.parent.size()) - 1;
    if (s < 1 || s > n || t < 1 || t > n || r.parent[s][t] == -1) return path;
    for (int v = t; v != 0; v = r.parent[s][v]) {
        path.push_back(v);
        if (static_cast<int>(path.size()) > n) return {}; // ciclo (resultado invalido)
    }
    std::reverse(path.begin(), path.end());
    return path;
}
