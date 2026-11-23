#include "Graph.hpp"

#include <iostream>
#include <string>
#include <vector>

// -----------------------------------------------------------------------
// Programa de exemplo / teste da biblioteca de grafos.
//
// Uso:
//   ./graph_program <arquivo_de_entrada> <matrix|list> [vertice_inicial_busca] [--directed]
//
// Gera:
//   output_info.txt   -> numero de vertices/arestas, graus, componentes
//   output_bfs.txt    -> arvore de busca em largura (pai e nivel)
//   output_dfs.txt    -> arvore de busca em profundidade (pai e nivel)
//   output_dijkstra.txt -> distancia e pai (Dijkstra) a partir de vertice_inicial
//   output_bellmanford.txt -> distancia e proximo vertice de todos ate vertice_inicial
// -----------------------------------------------------------------------
int main(int argc, char* argv[]) {
    // "--directed" pode aparecer em qualquer posicao; os demais argumentos
    // sao posicionais.
    bool directed = false;
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--directed") directed = true;
        else args.push_back(a);
    }
    if (args.size() < 2) {
        std::cerr << "Uso: " << argv[0]
                  << " <arquivo_de_entrada> <matrix|list> [vertice_inicial] [--directed]\n";
        return 1;
    }

    std::string inputFile = args[0];
    std::string repArg = args[1];
    int startVertex = (args.size() >= 3) ? std::stoi(args[2]) : 1;

    Graph::Representation rep;
    if (repArg == "matrix") {
        rep = Graph::Representation::MATRIX;
    } else if (repArg == "list") {
        rep = Graph::Representation::LIST;
    } else {
        std::cerr << "Representacao invalida. Use 'matrix' ou 'list'.\n";
        return 1;
    }

    try {
        Graph g(rep);
        g.loadFromFile(inputFile, directed);

        std::cout << "Grafo carregado: " << g.getNumVertices() << " vertices, "
                  << g.getNumEdges() << " arestas ("
                  << (g.isDirected() ? "direcionado" : "nao direcionado") << ").\n";

        g.writeGraphInfo("output_info.txt");
        std::cout << "Informacoes gerais escritas em output_info.txt\n";

        g.bfs(startVertex, "output_bfs.txt");
        std::cout << "Arvore de BFS (a partir do vertice " << startVertex
                  << ") escrita em output_bfs.txt\n";

        g.dfs(startVertex, "output_dfs.txt");
        std::cout << "Arvore de DFS (a partir do vertice " << startVertex
                  << ") escrita em output_dfs.txt\n";

        int diamExact = g.diameter();
        int diamApprox = g.diameterApprox();
        std::cout << "Diametro exato: " << diamExact << "\n";
        std::cout << "Diametro aproximado (double sweep): " << diamApprox << "\n";

        // Parte 2: Dijkstra (so se o grafo tiver pesos nao-negativos).
        if (g.hasNegativeWeights()) {
            std::cout << "Grafo com pesos negativos: Dijkstra nao se aplica "
                         "(veja Bellman-Ford/Floyd-Warshall abaixo).\n";
        } else {
            g.dijkstra(startVertex, "output_dijkstra.txt", Graph::DijkstraMode::HEAP);
            std::cout << "Caminhos minimos (Dijkstra com heap, a partir de "
                      << startVertex << ") escritos em output_dijkstra.txt\n";
        }

        // Parte 3: Bellman-Ford (todos -> startVertex) e Floyd-Warshall.
        {
            Graph::BellmanFordResult bf = g.bellmanFord(startVertex);
            if (bf.negativeCycle) {
                std::cout << "Bellman-Ford: ciclo negativo detectado.\n";
            } else {
                g.bellmanFord(startVertex, "output_bellmanford.txt");
                std::cout << "Bellman-Ford (todos -> " << startVertex << ", "
                          << bf.rounds << " rodadas) escrito em output_bellmanford.txt\n";
            }
            // Floyd-Warshall usa O(n^2) de memoria: so roda no demo para n pequeno.
            if (g.getNumVertices() <= 2000) {
                Graph::AllPairsResult fw = g.floydWarshall();
                std::cout << "Floyd-Warshall: "
                          << (fw.negativeCycle ? "ciclo negativo detectado" : "sem ciclo negativo")
                          << "\n";
            } else {
                std::cout << "Floyd-Warshall pulado no demo (n > 2000); use case_study3.\n";
            }
        }

        if (g.getNumVertices() >= 2) {
            int u = 1, v = g.getNumVertices();
            std::cout << "Distancia entre " << u << " e " << v << ": "
                      << g.distance(u, v) << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
