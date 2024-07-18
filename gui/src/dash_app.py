import os
import json
import numpy as np
import networkx as nx
import dash
from dash import dcc, html, Input, Output
from dash.exceptions import PreventUpdate
import plotly.graph_objects as go
import plotly.express as px

def create_graph_from_json(data):
    """
    This create a NetworkX graph starting from a json object with the following shape:
    ```
    {
        'nodes': [
            {
                "id": 0,
                "name": "ADAS_Manager_1",
                "reference": "invalid",
                "type": "component"
            },
            {
                "id": 1,
                "name": "TCS/PMCDeviceConnector_1",
                "reference": "invalid",
                "type": "component"
            }
        ],
     'edges': [
            {
                "source": 1,
                "target": 144
            },
            {
                "source": 2,
                "target": 145
            }
        ],
    }
    ```
    """
    G = nx.Graph()
    # Aggiungi nodi e archi al grafo in base ai dati JSON
    # Esempio:
    for node in data['nodes']:
        G.add_node(node['id'], label=node['name'], ntype=node['type'],
                   reference=node['reference'])
    for edge in data['edges']:
        G.add_edge(edge['source'], edge['target'])
    return G


def add_arc(posx, posy, radius):
    # radius = radius/2 * 0.001 # Moltiplica per 10 solo per visualizzazione
    radius = radius # Moltiplica per 10 solo per visualizzazione
    # theta = np.linspace(0, np.pi, 100)
    theta = np.linspace(0, np.pi*2, 100)
    x = posx + radius * np.cos(theta)
    y = posy + radius * np.sin(theta)
    # Aggiungi traccia per la semicirconferenza
    # Crea la forma della semicirconferenza
    # semi_circle_shape = go.layout.Shape(type="path",
    #                                     path=f"M {' '.join(map(str, x))} L {' '.join(map(str, y[::-1]))} Z",
    #                                     line=dict(color='purple', width=3), name='Semi-circle-shape')
    
    semi_circle_trace = go.Scatter(x=x, y=y, mode='lines', line=dict(color='purple', width=3), name='Semi-circle')
    # semi_circle_trace.update(
    #     marker=dict(size=25)
    # )
    return semi_circle_trace

def draw_graph(acs_graph):
    # Crea il grafo da JSON
    graph = create_graph_from_json(acs_graph)
    # Crea elenco di nodi e archi
    nodes = list(graph.nodes())
    edges = list(graph.edges())
    # Imposta posizioni dei nodi
    pos = nx.spring_layout(graph, seed=0)
    # Crea lista di tracce per i nodi
    node_traces = []
    annotations = []
    for node in nodes:
        current_node = acs_graph['nodes'][node]
        posx, posy = pos[node]
        if current_node['type'] == 'component':
            # Aggiungi un pallino verde status ok
            reference = current_node['reference']
            node_traces.append(
                go.Scatter(x=[posx], y=[posy], mode='markers', 
                           marker=dict(size=25, 
                                       color='green' if reference == 'ok' else 'red'
                                       ), hoverinfo='none', showlegend=False))
        node_trace = go.Scatter(x=[posx], y=[posy], mode='markers+text', 
                                marker=dict(size=50 if current_node['type'] == 'container' else 20, 
                                            color='skyblue' if current_node['type'] == 'container' else 'orange'), 
                                text=current_node['name'],
                                hoverinfo='text',
                                textposition='bottom center', textfont=dict(color='black'), showlegend=False)
        # print(f'|{node_trace.marker.size}|')
        node_traces.append(node_trace)
        # Calcola il raggio della semicirconferenza in base a un attributo del nodo (ad esempio, 'size')
        # semi_circle_trace = add_arc(posx, posy, 25)
        # Aggiungi traccia per la semicirconferenza
        # node_traces.append(semi_circle_trace)
        # Text inside nodes
        annotations.append(dict(x=pos[node][0],
                                y=pos[node][1],
                                xref='x',
                                yref='y',
                                text=f'{current_node["id"]}',
                                showarrow=False,
                                font=dict(
                                    size=14,
                                    color="black")))
        
    # Crea traccia per gli archi
    edge_trace = go.Scatter(x=[], y=[], mode='lines', line=dict(width=0.5, color='gray'), hoverinfo='none', showlegend=False)
    for edge in edges:
        x0, y0 = pos[edge[0]]
        x1, y1 = pos[edge[1]]
        edge_trace['x'] += tuple([x0, x1, None])
        edge_trace['y'] += tuple([y0, y1, None])
    # Crea layout
    layout = go.Layout(title='<b>Components and Containers graph</b>', title_font_size=24, title_x=0.5, showlegend=True, hovermode='closest',
                       margin=dict(b=20, l=5, r=5, t=40),
                       xaxis=dict(showgrid=False, zeroline=False, showticklabels=False),
                       yaxis=dict(showgrid=False, zeroline=False, showticklabels=False),
                       width=1800, height=1800,
                       annotations=annotations)
    # Crea figura
    fig = go.Figure(data=node_traces + [edge_trace], layout=layout)
    # Aggiungi voci alla legenda
    fig.add_trace(go.Scatter(x=[None], y=[None], mode='markers', marker=dict(size=20, 
                                                                             color='skyblue'), showlegend=True, name='Container'))
    fig.add_trace(go.Scatter(x=[None], y=[None], mode='markers', marker=dict(size=20, 
                                                                             color='orange'), showlegend=True, name='Component'))
    fig.add_trace(go.Scatter(x=[None], y=[None], mode='markers', marker=dict(size=20, 
                                                                             color='white', 
                                                                             line=dict(width=2,
                                                                                       color='red')), showlegend=True, name='reference: invalid'))
    fig.add_trace(go.Scatter(x=[None], y=[None], mode='markers', marker=dict(size=20, color='white', 
                                                                             line=dict(width=2,
                                                                                       color='green')), showlegend=True, name='reference: ok'))
    # Ritorna la figura
    return fig


# Inizializza l'app Dash
app = dash.Dash(__name__)

# Layout dell'app
app.layout = html.Div([
    # html.Button('Aggiorna', id='update-button'),  # Pulsante di aggiornamento
    html.Div(id='graph-container'),
    dcc.Interval(id='interval-component', interval=5*1000, n_intervals=0)  # Intervallo di aggiornamento ogni 30 secondi
])

# Callback per aggiornare il grafico
@app.callback(
    Output('graph-container', 'children'),  # Output della callback
    [Input('interval-component', 'n_intervals')]    # Input della callback (ad esempio un pulsante di aggiornamento)
)
def update_graph(n_clicks):
    if n_clicks is None:
        raise PreventUpdate  # Se non è stato cliccato alcun pulsante, non aggiornare il grafico

    # Definisci il percorso del file JSON
    file_path = os.path.join(os.getcwd(), '..', 'data' , 'acs_graph.json')
    # Carica il file JSON
    with open(file_path, "r") as file:
        acs_graph = json.load(file)

    # Chiama la funzione per disegnare il grafico e ottieni i byte dell'immagine
    graph_image = draw_graph(acs_graph)

    # Restituisci l'immagine come elemento HTML per matplotlib
    # return html.Img(src='data:image/png;base64,{}'.format(base64.b64encode(graph_image).decode()))
    # Restituisci l'immagine come elemento HTML per plotly
    return dcc.Graph(figure=graph_image)


# Esegui l'app
if __name__ == '__main__':
    host = '0.0.0.0'
    port = 8051
    app.run_server(host=host, port=port, debug=True)
