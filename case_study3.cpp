#include "Graph.hpp"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// case_study3 -- estudos de caso da Parte 3 (grafos direcionados com pesos,
// Bellman-Ford e Floyd-Warshall).
//
// Uso:
//   ./case_study3 <grafo> [--directed] [--rep=list|matrix] [--runs=10]
//                 [--seed=42] [--skip-fw] [--fw-max-n=10000]
//                 [--bf-all-sample=K]
//
//  Q1. Bellman-Ford: distancia dos vertices 10, 20 e 30 ate o vertice 100.
//  Q2. Floyd-Warshall: distancias entre os pares de {10,20,30,40,50}.
//  Q3. Tempo medio (em segundos, sem leitura/escrita em disco) de cada
//      algoritmo: BFS, DFS, Dijkstra (heap), Bellman-Ford e Floyd-Warshall.
//  Q4. Bellman-Ford para todos os pares (n execucoes, uma por destino) x
//      Floyd-Warshall.
//
// --bf-all-sample=K: em vez de rodar o Bellman-Ford para os n destinos em
// cada rodada, roda so para K destinos aleatorios e EXTRAPOLA (x n/K). O
// resultado e marcado como estimativa. Use so se o all-pairs completo for
// demorado demais.
// ---------------------------------------------------------------------------

using Clock = std::chrono::steady_clock;

static void sep() {
    std::cout << "----------------------------------------------------------\n";
}

static std::string fmtDist(double d) {
    if (std::isinf(d)) return "inf";
    std::ostringstream os;
    os << d;
    return os.str();
}

static std::string pathStr(const std::vector<int>& p) {
    std::string s;
    for (size_t i = 0; i < p.size(); ++i) {
        if (i) s += " -> ";
        s += std::to_string(p[i]);
    }
    return s;
}

// Media (em segundos) de `runs` execucoes de fn(i).
template <typename Fn>
static double avgSeconds(int runs, Fn fn) {
    double total = 0.0;
    for (int i = 0; i < runs; ++i) {
        auto t0 = Clock::now();
        fn(i);
        auto t1 = Clock::now();
        total += std::chrono::duration<double>(t1 - t0).count();
    }
    return total / runs;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0]
                  << " <grafo> [--directed] [--rep=list|matrix] [--runs=10]"
                     " [--seed=42] [--skip-fw] [--fw-max-n=10000]"
                     " [--bf-all-sample=K]\n";
        return 1;
    }
    std::string filename = argv[1];
    bool directed = false, skipFW = false;
    int runs = 10, bfSample = 0;
    long long fwMaxN = 10000;
    unsigned seed = 42;
    Graph::Representation rep = Graph::Representation::LIST;

    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--directed") directed = true;
        else if (a == "--undirected") directed = false;
        else if (a == "--skip-fw") skipFW = true;
        else if (a == "--rep=matrix") rep = Graph::Representation::MATRIX;
        else if (a == "--rep=list") rep = Graph::Representation::LIST;
        else if (a.rfind("--runs=", 0) == 0) runs = std::stoi(a.substr(7));
        else if (a.rfind("--seed=", 0) == 0) seed = (unsigned)std::stoul(a.substr(7));
        else if (a.rfind("--fw-max-n=", 0) == 0) fwMaxN = std::stoll(a.substr(11));
        else if (a.rfind("--bf-all-sample=", 0) == 0) bfSample = std::stoi(a.substr(16));
        else { std::cerr << "Argumento nao reconhecido: " << a << "\n"; return 1; }
    }

    try {
        Graph g(rep);
        g.loadFromFile(filename, directed);
        const int n = g.getNumVertices();
        std::cout << "Grafo: " << n << " vertices, " << g.getNumEdges() << " arestas, "
                  << (g.isDirected() ? "DIRECIONADO" : "nao direcionado") << ", "
                  << (g.isWeighted() ? "com pesos" : "SEM pesos")
                  << (g.hasNegativeWeights() ? " (pesos NEGATIVOS)" : "") << "\n";

        std::cout << std::fixed << std::setprecision(6);

        // ---------------- Q1: Bellman-Ford ate o vertice 100 --------------
        sep();
        std::cout << "Q1: Bellman-Ford, distancia de 10, 20 e 30 ate o vertice 100\n";
        sep();
        if (n < 100) {
            std::cout << "[Aviso] grafo com menos de 100 vertices; nao se aplica.\n";
        } else {
            Graph::BellmanFordResult bf = g.bellmanFord(100);
            if (bf.negativeCycle) {
                std::cout << "Ciclo negativo detectado: distancias nao definidas.\n";
            } else {
                std::cout << "(rodadas executadas: " << bf.rounds << ")\n";
                std::cout << "| origem | distancia ate 100 | caminho |\n";
                for (int v : {10, 20, 30}) {
                    std::cout << "| " << v << " | " << fmtDist(bf.dist[v]) << " | "
                              << pathStr(Graph::pathToTarget(bf, v)) << " |\n";
                }
            }
        }

        // ---------------- Q2: Floyd-Warshall ------------------------------
        Graph::AllPairsResult fw;
        sep();
        std::cout << "Q2: Floyd-Warshall, distancias entre {10,20,30,40,50}\n";
        sep();
        double estGB = (double)(n + 1) * (n + 1) * (sizeof(double) + sizeof(int)) / 1e9;
        if (skipFW) {
            std::cout << "(pulado por --skip-fw)\n";
        } else if (n > fwMaxN) {
            std::cout << "[Info] n = " << n << " > --fw-max-n=" << fwMaxN
                      << ": Floyd-Warshall precisaria de ~" << estGB
                      << " GB (matrizes de distancia e pai). Pulado.\n";
        } else if (n < 50) {
            std::cout << "[Aviso] grafo com menos de 50 vertices; nao se aplica.\n";
        } else {
            fw = g.floydWarshall();
            if (fw.negativeCycle) {
                std::cout << "Ciclo negativo detectado: distancias nao definidas.\n";
            } else {
                const int S[5] = {10, 20, 30, 40, 50};
                std::cout << "| de \\ para |";
                for (int t : S) std::cout << " " << t << " |";
                std::cout << "\n";
                for (int s : S) {
                    std::cout << "| " << s << " |";
                    for (int t : S) std::cout << " " << fmtDist(fw.dist[s][t]) << " |";
                    std::cout << "\n";
                }
                // Sanidade: Bellman-Ford ate cada destino deve concordar.
                bool same = true;
                for (int t : S) {
                    Graph::BellmanFordResult bf = g.bellmanFord(t);
                    for (int s : S) {
                        double a = fw.dist[s][t], b = bf.dist[s];
                        if (std::isinf(a) != std::isinf(b) ||
                            (!std::isinf(a) && std::fabs(a - b) > 1e-9 * (1 + std::fabs(a))))
                            same = false;
                    }
                }
                std::cout << "Checagem: FW == Bellman-Ford nesses pares? "
                          << (same ? "sim" : "NAO") << "\n";
            }
        }

        // Libera as matrizes do Q2 (~12*n^2 bytes) ANTES do Q3: cada rodada de
        // Floyd-Warshall do Q3 aloca outro par de matrizes, e sem isto o pico
        // de memoria seria o dobro.
        fw = Graph::AllPairsResult();

        // ---------------- Q3: tempos medios ------------------------------
        sep();
        std::cout << "Q3: tempo medio de execucao (" << runs << " rodadas, em segundos)\n";
        sep();
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> pick(1, n);
        std::vector<int> verts(runs);
        for (int& v : verts) v = pick(rng);

        std::cout << "| algoritmo | tempo medio (s) |\n";
        double dummy = 0; // evita que o compilador elimine as chamadas
        double tBFS = avgSeconds(runs, [&](int i) { dummy += g.bfs(verts[i]).level[1]; });
        double tDFS = avgSeconds(runs, [&](int i) { dummy += g.dfs(verts[i]).level[1]; });
        std::cout << "| BFS | " << tBFS << " |\n";
        std::cout << "| DFS | " << tDFS << " |\n";
        if (g.hasNegativeWeights()) {
            std::cout << "| Dijkstra (heap) | n/a (pesos negativos) |\n";
        } else {
            double t = avgSeconds(runs, [&](int i) {
                dummy += g.dijkstra(verts[i], Graph::DijkstraMode::HEAP).dist[verts[i]];
            });
            std::cout << "| Dijkstra (heap) | " << t << " |\n";
        }
        double tBF = avgSeconds(runs, [&](int i) {
            dummy += g.bellmanFord(verts[i]).rounds;
        });
        std::cout << "| Bellman-Ford (1 destino) | " << tBF << " |\n";

        double tFW = -1;
        if (skipFW || n > fwMaxN) {
            std::cout << "| Floyd-Warshall | n/a (pulado) |\n";
        } else {
            tFW = avgSeconds(runs, [&](int) {
                dummy += g.floydWarshall().negativeCycle;
            });
            std::cout << "| Floyd-Warshall | " << tFW << " |\n";
        }
        std::cout << "(origens/destinos aleatorios, seed " << seed << ")\n";

        // ---------------- Q4: BF all-pairs x FW ---------------------------
        sep();
        std::cout << "Q4: Bellman-Ford para todos os pares x Floyd-Warshall\n";
        sep();
        int sample = (bfSample > 0 && bfSample < n) ? bfSample : n;
        std::vector<int> targets(sample);
        if (sample == n) {
            for (int t = 0; t < n; ++t) targets[t] = t + 1;
        } else {
            for (int& t : targets) t = pick(rng);
        }
        double tBFall = avgSeconds(runs, [&](int) {
            for (int t : targets) dummy += g.bellmanFord(t).rounds;
        });
        if (sample != n) {
            tBFall *= (double)n / sample;
            std::cout << "[ESTIMATIVA] Bellman-Ford rodado para " << sample
                      << " destinos aleatorios e extrapolado x" << n << "/" << sample << "\n";
        }
        std::cout << "Bellman-Ford (n destinos): " << tBFall << " s (media de " << runs
                  << " rodadas)\n";
        if (tFW >= 0) {
            std::cout << "Floyd-Warshall           : " << tFW << " s\n";
            std::cout << "Razao BF-todos-pares / FW: " << tBFall / tFW << "x -> "
                      << (tBFall < tFW ? "Bellman-Ford foi mais rapido"
                                       : "Floyd-Warshall foi mais rapido")
                      << "\n";
        } else {
            std::cout << "Floyd-Warshall: nao executado (veja Q2/Q3).\n";
        }
        sep();
        if (dummy == -123456.789) std::cout << dummy; // nunca ocorre
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
