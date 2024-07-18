import os
import json
import dash
from dash import dcc, html, Input, Output, State
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

# Funzione per disegnare l'istogramma
def draw_histogram(selected_sector):
    # Questo è solo un esempio di istogramma, sostituiscilo con i tuoi dati reali
    x = [1, 2, 3, 4, 5]
    y = [10, 15, 13, 17, 18]
    # Crea il layout dell'istogramma
    layout = go.Layout(
        title=f"<b>Sector {selected_sector} Histogram</b>",
        xaxis=dict(title="X Axis"),
        yaxis=dict(title="Y Axis")
    )
    # Crea il tracciamento dell'istogramma
    histogram = go.Figure(
        data=[go.Bar(x=x, y=y)],
        layout=layout
    )
    return histogram


def draw_sunchart(datasunchart):
    # Define color scale based on reference attribute
    facecolors = {
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
            color_discrete_map=facecolors,
        ))
    fig.update_layout(margin = dict(t=0, l=0, r=0, b=0),width=1500, height=900,)
    # Definiamo i colori dei bordi dei settori
    border_colors = ['orange' if nodetype == 'component' else 'black' for nodetype in datasunchart['nodetype']]
    # Impostazione del colore del bordo
    fig.update_traces(marker=dict(line=dict(color=border_colors, width=2)))  # Cambia il colore del bordo in nero con larghezza 2
    # Custom legend
    custom_legend = []
    for ref, color in facecolors.items():
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


# Inizializza l'app Dash app-layout
app = dash.Dash(__name__)

# Layout dell'app
app.layout = html.Div([
    html.Div([
        dcc.Graph(id='sunburst-chart'),
        html.Div(id='histogram-container')
    ]),
    dcc.Interval(id='interval-component', interval=30*1000, n_intervals=0),  # Intervallo di aggiornamento ogni 30 secondi
])

# Callback per aggiornare l'istogramma quando un settore del sunburst chart viene cliccato
@app.callback(
    Output('histogram-container', 'children'),
    [Input('sunburst-chart', 'clickData')],
)
def update_histogram(click_data):
    print(f'action: selected {click_data}')
    if click_data is None:
        # raise PreventUpdate
        return dash.no_update
    # Verifica se ci sono dati cliccati e recupera le informazioni necessarie
    clicked_point = click_data.get('points', [])
    if not clicked_point:
        raise None
    selected_sector = clicked_point[0].get('label', None)
    if selected_sector is None:
        raise PreventUpdate
    # print(f'action: selected {selected_sector}')
    # Disegna e restituisci l'istogramma
    histogram = draw_histogram(selected_sector)
    return dcc.Graph(figure=histogram)


# Callback per aggiornare il sunburst chart quando vengono caricati nuovi dati
@app.callback(
    Output('sunburst-chart', 'figure'),
    [Input('interval-component', 'n_intervals')]
)
def update_sunburst(n_intervals):
    if n_intervals is None:
        raise PreventUpdate

    # Leggi i nuovi dati dal file JSON
    file_path = os.path.join(os.getcwd(), '..', 'data' , 'acs_json.json')
    with open(file_path, "r") as file:
        acs_graph = json.load(file)

    # Estrai e disegna il sunburst chart
    datasunchart = extract_sunburstchart(acs_graph)
    sunburst_chart = draw_sunchart(datasunchart)
    print('action: sunburstchart_updated')
    return sunburst_chart

# Esegui l'app
if __name__ == '__main__':
    host = '0.0.0.0'
    port = 8050
    app.run_server(host=host, port=port, debug=True)
