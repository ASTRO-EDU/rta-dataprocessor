import os
import json
import dash
from dash import dcc, html, Input, Output
from dash.exceptions import PreventUpdate
import plotly.graph_objects as go
import plotly.express as px

def extract_sunburstchart(data):
    # Get components 
    components = data['Summary']['COMPONENTS']
    componentskeys = list(components.keys())
    print(type(componentskeys[0]))
    # Get containers
    containers = data['Summary']['CONTAINERS']
    containerskeys = list(containers.keys())
    # Containers missing
    containersmissing = []
    for cmp in componentskeys:
        cmp_cntname = components[cmp]['container_name']
        if not cmp_cntname in containerskeys and  not cmp_cntname in containersmissing:
            containersmissing.append(str(components[cmp]['container_name']))
    # Create data dict
    datasunchart = dict(
        node=["containers"] + [str(components[cmp]['name']) for cmp in componentskeys] + [str(containers[cnt]['name']) for cnt in containerskeys] + containersmissing,
        parent=[""] + [str(components[cmp]['container_name']) for cmp in componentskeys] + ["containers" for _ in containerskeys] + ["containers" for _ in containersmissing],
        value=[1] + [1 for _ in componentskeys] + [1 for _ in containerskeys] + [1 for _ in containersmissing],
        reference=["root"] + [components[cmp]['reference'] for cmp in componentskeys] + [containers[cnt]['reference'] for cnt in containerskeys] + ["missing_from_json" for _ in containersmissing],
        nodetype=["root"] + ["component" for _ in componentskeys] + ["container" for _ in containerskeys] + ["container" for _ in containersmissing],
        color=["white"] + ['lightgreen' if components[cmp]['reference'] == 'valid' else 'red' for cmp in componentskeys] \
                        + ['darkgreen' if containers[cnt]['reference'] == 'ok' else 'black' for cnt in containerskeys] + ['gray' for _ in containersmissing]
        )
    return datasunchart

def draw_sunchart(datasunchart):
    print(datasunchart) 
    # Define color scale based on reference attribute
    colors = {
        'root': 'skyblue',
        'valid': 'lightgreen',
        'ok': 'darkgreen',
        'invalid': 'red',
        'missing_from_json': 'gray'
    }
    fig = go.Figure(px.sunburst(
            datasunchart,
            names='node',
            parents='parent',
            values='value',
            color='reference',
            color_discrete_map=colors
        ))
    fig.update_layout(margin = dict(t=0, l=0, r=0, b=0),width=1600, height=900,)
    # Custom legend
    custom_legend = []
    for ref, color in colors.items():
        custom_legend.append(
            go.Scatter(
                x=[None],
                y=[None],
                mode='markers',
                marker=dict(size=10, color=color),
                showlegend=True,
                name=ref
            )
        )
    # Add custom legend to the figure
    fig.add_traces(custom_legend)
    # Ritorna la figura
    return fig


# Inizializza l'app Dash
app = dash.Dash(__name__)

# Layout dell'app
app.layout = html.Div([
    # html.Button('Aggiorna', id='update-button'),  # Pulsante di aggiornamento
    html.Div(id='graph-container'),
    dcc.Interval(id='interval-component', interval=30*1000, n_intervals=0)  # Intervallo di aggiornamento ogni 30 secondi
])

# Callback per aggiornare il grafico
@app.callback(
    Output('graph-container', 'children'),  # Output della callback
    [Input('interval-component', 'n_intervals')]    # Input della callback (ad esempio un pulsante di aggiornamento)
)
def update_graph(n_clicks):
    # Definisci il percorso del file JSON
    file_path = os.path.join(os.getcwd(), '..', 'data' , 'acs_json.json')
    # Carica il file JSON
    with open(file_path, "r") as file:
        acs_graph = json.load(file)
    data = extract_sunburstchart(acs_graph)
    # Chiama la funzione per disegnare il grafico 
    sunburstchart_image = draw_sunchart(data)
    # Restituisci l'immagine come elemento HTML per plotly
    return dcc.Graph(figure=sunburstchart_image)


# Esegui l'app
if __name__ == '__main__':
    host = '0.0.0.0'
    port = 8050
    app.run_server(host=host, port=port, debug=True)
