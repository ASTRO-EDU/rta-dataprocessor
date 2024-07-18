import os
import json
import dash
from dash import dcc, html, Input, Output, State
from dash.exceptions import PreventUpdate
import plotly.graph_objects as go
import plotly.express as px

prev_interval = -1

def extract_dataSunBurstChart(data):
    # Get components 
    components = data['Summary']['COMPONENTS']
    componentskeys = list(components.keys())
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

###########################################################################################################################
###########################################################################################################################
# Funzione per disegnare l'istogramma
def draw_histogram(selected_sector):
    # Questo è solo un esempio di istogramma, sostituiscilo con i tuoi dati reali
    x = [1, 2]
    y = [10, 15]
    # Crea il layout dell'istogramma
    layout = go.Layout(
        title=f"<b>Sector {selected_sector} Histogram</b>",
        xaxis=dict(title="X Axis"),
        yaxis=dict(title="Y Axis"),
        width=600, height=600
    )
    # Crea il tracciamento dell'istogramma
    histogram = go.Figure(
        data=[go.Bar(x=x, y=y, text=['cpu_usage', 'memory_usage'])],
        layout=layout
    )
    return histogram
###############################################################################################
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
            color_discrete_map=facecolors
        ))
    fig.update_layout(margin=dict(t=0, l=0, r=0, b=0), 
                      width=1500, height=900)
    # Definiamo i colori dei bordi dei settori
    border_colors = ['orange' if nodetype == 'component' else 'black' for nodetype in datasunchart['nodetype']]
    border_width  = [2 if nodetype == 'component' else 4 for nodetype in datasunchart['nodetype']]
    # Impostazione del colore del bordo
    fig.update_traces(marker=dict(line=dict(color=border_colors, width=border_width)))  # Cambia il colore del bordo in nero con larghezza 2
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
    # # Add custom legend to the figure
    fig.add_traces(custom_legend)
    # Ritorna la figura
    return fig
###########################################################################################################################
###########################################################################################################################

# Inizializza l'app Dash app-layout
app = dash.Dash(__name__)
# Layout dell'app
app.layout = html.Div([
    html.Div(id='sunburst-container', children=[
        html.Button('Mostra Istogramma', id='hidehist-button', n_clicks=0, 
                    style={'position': 'fixed', 'top': 10, 'z-index': '1',
                           'font-size': '20px', 'padding': '10px'}),
        html.Label('none', id='label-histname', style={'display': 'none'}),
        dcc.Graph(id='sunburst-chart'),
        html.Div(id='histogram-container', children=[
            dcc.Graph(id='histogram-chart', style={'display': 'none'})
            ])
    ]),
    dcc.Interval(id='interval-component', interval=30*1000, n_intervals=0),  # Intervallo di aggiornamento ogni 30 secondi
])

###########################################################################################################################
###########################################################################################################################
# Callback per rendere invisibile l'istogramma al primo accesso
@app.callback(
    [Output('histogram-chart', 'style'),
     Output('hidehist-button', 'children'),
     Output('label-histname', 'children')],
    [Input('hidehist-button', 'n_clicks'),
     Input('sunburst-chart', 'clickData'),
     Input('interval-component', 'n_intervals')],
    [State('histogram-chart', 'style'),
     State('hidehist-button', 'children'),
     State('label-histname', 'children')]
)
def update_histogram_visibility(n_clicks, click_data, n_intervals, histogram_style, button_text, label_text):
    if click_data == None:
        raise PreventUpdate 
    global prev_interval
    # global prev_nclicks
    # if prev_interval < n_intervals:
    #     prev_interval = n_intervals
    #     return {'display': 'none'}, 'Mostra Istogramma', 'none'
    # Verifica che l'istogramma venga plottato solo nelle foglie
    clicked_point = click_data.get('points', [])
    selected_path = clicked_point[0].get('currentPath', None)
    if (not clicked_point) or (not selected_path) or len((selected_path.split('/'))) <= 2:
        raise PreventUpdate
    # La visibilità dell'istogramma, dopo aver selezionato almeno un component, 
    #   va cambiata se e solo se viene cliccato il bottone
    if label_text != 'none' and str(n_clicks) == label_text:
        raise PreventUpdate
    if button_text == "Mostra Istogramma":  # Se il pulsante è stato premuto un numero dispari di volte
        # Rendi visibile l'istogramma e cambia il testo del pulsante a "Nascondi Istogramma"
        histogram_style = {'display': 'block'}
        button_text = 'Nascondi Istogramma'
        label_text = str(n_clicks)
    else:
        # Altrimenti, rendi invisibile l'istogramma e cambia il testo del pulsante a "Mostra Istogramma"
        histogram_style = {'display': 'none'}
        button_text = 'Mostra Istogramma'
        label_text = 'none'
    return histogram_style, button_text, label_text
###############################################################################################
# Callback per aggiornare l'istogramma quando un settore del sunburst chart viene cliccato
@app.callback(
    Output('histogram-chart', 'figure'),
    [Input('sunburst-chart', 'clickData')]
)
def update_histogram(click_data):
    if click_data is None:
        raise PreventUpdate
    # Verifica che l'istogramma venga plottato solo nelle foglie
    clicked_point = click_data.get('points', [])
    selected_path = clicked_point[0].get('currentPath', None)
    #
    if (not clicked_point) or (not selected_path) or len((selected_path.split('/'))) <= 2:
        raise PreventUpdate
    # Estrae la label
    selected_sector = clicked_point[0].get('label', None)
    if selected_sector is None:
        raise PreventUpdate
    # Disegna e restituisci l'istogramma
    histogram = draw_histogram(selected_sector)
    return histogram
###############################################################################################
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
    datasunchart = extract_dataSunBurstChart(acs_graph)
    sunburst_chart = draw_sunchart(datasunchart)
    print('action: sunburstchart_updated')
    return sunburst_chart
###########################################################################################################################
###########################################################################################################################

# Esegui l'app
if __name__ == '__main__':
    host = '0.0.0.0'
    port = 8050
    app.run_server(host=host, port=port, debug=True)
