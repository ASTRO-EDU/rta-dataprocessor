import json
import os

def get_nodes(components, containers):
    acs_graph = {
        "nodes": [],
        "edges": [],
    }
    # get component keys and container keys
    components_keys = list(components.keys())
    containers_keys = list(containers.keys())
    nodes_keys = components_keys + containers_keys
    # id = 0
    # # create nodes
    id = 0
    for k in components_keys:
        acs_graph['nodes'].append({
            "id": id,
            "name": k,
            "type": "component",
            "reference": components[k]['reference']
        })
        if components[k]['container_name'] in containers_keys:
            acs_graph['edges'].append({
                "source": id,
                "target": containers_keys.index(components[k]['container_name']) + len(components_keys),
            })
        id += 1
    # containers
    for k in containers_keys:
        acs_graph['nodes'].append({
            "id": id,
            "name": k,
            "type": "container",
            "reference": containers[k]['reference']
        })
        id += 1
    return acs_graph



def main():
    # Definisci il percorso del file JSON
    file_path = os.path.join(os.getcwd(), '..', 'data' , 'acs_json.json')
    # Carica il file JSON
    with open(file_path, "r") as file:
        data = json.load(file)
    # Summary part
    summ = data['Summary']
    # Clients part
    clients = summ['CLIENTS']
    # Components part
    components = summ['COMPONENTS']
    # Containers part
    containers = summ['CONTAINERS']
    # Transform it in a graph shape
    acs_graph = get_nodes(components, containers)
    # Salva dizionario come json
    with open(os.path.join(os.getcwd(), '..', 'data', 'acs_graph.json'), "w") as file:
        json.dump(acs_graph, file)


if __name__ == '__main__':
    main()