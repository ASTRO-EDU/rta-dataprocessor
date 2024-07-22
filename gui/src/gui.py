import os
import json
import dash
from dash import dcc, html, Input, Output, State
from dash.exceptions import PreventUpdate
import dash_bootstrap_components as dbc
import plotly.graph_objects as go
import plotly.express as px
import zmq
import threading
from queue import Queue
import queue 

prev_interval = -1
prev_nclicks  = 'none'

class Block:

    def __init__(self,name,parent,value,status,nodetype,monitoring_data=None):
        self.name = name
        self.parent = parent
        self.value = value
        self.status = status
        self.nodetype = nodetype
        self.monitoring_data = monitoring_data


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
        '2': 'skyblue',
        '0': 'lightgreen',
        '1': 'darkgreen',
        'invalid': 'red',
        'null': 'gray'
    }
    fig = go.Figure(px.sunburst(
            datasunchart,
            names='node',
            parents='parent',
            branchvalues="total",
            #values='value',
            color='reference',
            color_discrete_map=facecolors
        ))
        # Update layout to make the chart responsive
    fig.update_layout(
        margin=dict(t=0, l=0, r=0, b=0),
        autosize=True
    )
    
    # Definiamo i colori dei bordi dei settori
    border_colors = ['orange' if nodetype == 'component' else 'black' for nodetype in datasunchart['nodetype']]
    border_width  = [2 if nodetype == 'component' else 4 for nodetype in datasunchart['nodetype']]
    # Impostazione del colore del bordo
    fig.update_traces(
        marker=dict(line=dict(color=border_colors, width=border_width)),
        hovertemplate='<b>%{label}</b><br>Status: %{color}<br><extra></extra>',
        customdata=[datasunchart['nodetype']]
        )  # Cambia il colore del bordo in nero con larghezza 2
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

###############################################################################################

def create_chart_input_from_dict(block_array):

    node_list = []
    parent_list = []
    value_list = []
    reference_list = []
    nodetype_list = []

    for block in block_array:
        node_list.append(block.name)
        parent_list.append(block.parent)
        value_list.append(block.value)
        reference_list.append(block.status)
        nodetype_list.append(block.nodetype)

    return {"node": node_list, 
            "parent": parent_list, 
            "value": value_list ,
            "reference": reference_list,
            "nodetype": nodetype_list
           }
        

def initialize_data_dict(baseconfig_path):

    base_dict = {}

    with open(baseconfig_path, 'r') as f:
        data = json.load(f)
    
    dataprocessor_names = data["dataprocessor_names"]
    workermanager_names = data["workermanager_names"]
    workers = data["workers"]
    monitoring_socket = data["monitoring_socket"]

    print("Data Processor Names:", dataprocessor_names)
    print("Worker Manager Names:", workermanager_names)
    print("Workers:", workers)
    print("Monitoring Socket:", monitoring_socket)

    block_array = []

    for element in dataprocessor_names:
        block = Block(element,"",1,"null","dataprocessor")
        block_array.append(block)

    for element in workermanager_names:
        block = Block(element,element.split("-")[0],1,"null","workermanager")
        block_array.append(block)

    for i in range(0,len(workers)):

        workers_number = workers[i]

        for j in range(0,workers_number):
            
            block = Block(workermanager_names[i]+"-worker-"+str(j),workermanager_names[i],1,"null","worker")
            block_array.append(block)

             

    return {'block_array':block_array,'monitoring_socket':monitoring_socket}



def update_block_array(block_array,new_dict):

    print("#")
    print(new_dict['header']['pidsource'])
    print("#")
    dp_name = new_dict['header']['pidsource'].split("-")[1]
    wm_name = new_dict['header']['pidsource'].split("-")[2]

    wm_status = new_dict['status']

    dp_status = "null"

    if wm_status == "Waiting":
        wm_status = "1"
    elif wm_status == "Processing":
        wm_status = "2"
    else:
        wm_status = "invalid"

    #wm_status = int(new_dict['workerstatus'])

    wm_queue_lp = new_dict['queue_lp_size']
    wm_queue_hp = new_dict['queue_hp_size']

    wm_queue_results_lp = new_dict['queue_lp_result_size']
    wm_queue_results_hp = new_dict['queue_hp_result_size']

    worker_rates = new_dict['worker_rates']
    worker_tot_events = new_dict['worker_tot_events']
    worker_status = new_dict['worker_status']

    for block in block_array:

        if block.name==dp_name:
            block.status=str(dp_status)
            print("update dp status")

        if block.name==dp_name+"-"+wm_name:
            block.status=str(wm_status)
            print("update wm status")
            block.monitoring_data={"wm_queue_lp":wm_queue_lp,
                                   "wm_queue_hp":wm_queue_hp,
                                   "wm_queue_results_lp":wm_queue_results_lp,
                                   "wm_queue_results_hp":wm_queue_results_hp
                                    }

    for key, value in worker_rates.items():
        for block in block_array:
            if block.name==dp_name+"-"+wm_name+"-worker-"+str(key):
                block.status=str(int(worker_status[key]))
                block.monitoring_data={"rate":worker_rates[key],"tot_event":worker_tot_events[key]}

    """
    {
    "header": {
        "type": 1,
        "time": 1721138612.3700526,
        "pidsource": "WorkerManager-RTADP1-Rate",
        "pidtarget": "*"
    },
    "status": "Waiting",  #status worker manager
    "procinfo": {
        "cpu_percent": 98.8,
        "memory_usage": [
            28909568,
            2090962944,
            12677120,
            4096,
            0,
            258654208,
            0
        ]
    },
    "queue_lp_size": 0,
    "queue_hp_size": 0,
    "stopdatainput": 0,
    "queue_lp_result_size": 0,
    "queue_hp_result_size": 0,
    "workerstatusinit": 5,
    "workerstatus": 0,
    "worker_rates": {
        "0": 0.0,
        "1": 0.0,
        "2": 0.0,
        "3": 0.0,
        "4": 0.0
    },
    "worker_tot_events": {
        "0": 0,
        "1": 0,
        "2": 0,
        "3": 0,
        "4": 0
    },
    "worker_status": {
        "0": 0,
        "1": 0,
        "2": 0,
        "3": 0,
        "4": 0
    }
}"""


###########################################################################################################################
###########################################################################################################################

# Inizializza l'app Dash app-layout
app = dash.Dash(__name__)
# Layout dell'app

app.layout = html.Div([
    dbc.Container([
        html.Div(id='sunburst-container', className='p-3', children=[
            html.Button('Mostra Istogramma', id='hidehist-button', n_clicks=0, className='btn btn-primary mb-3'),
            html.Label('none', id='label-histname', style={'display': 'none'}),
            dcc.Graph(id='sunburst-chart', className='mb-3'),
            html.Div(id='histogram-container', children=[
                dcc.Graph(id='histogram-chart', style={'display': 'none'})
            ])
        ])
    ], fluid=True),
    dcc.Interval(id='interval-component', interval=1000, n_intervals=0),  # Intervallo di aggiornamento ogni secondo
])
"""
app.layout = html.Div([
    html.Div(id='sunburst-container', children=[
        html.Button('Mostra Istogramma', id='hidehist-button', n_clicks=0, 
                    style={'font-size': '1.5rem', 'padding': '1rem'}),
        html.Label('none', id='label-histname', style={'display': 'none'}),
        dcc.Graph(id='sunburst-chart'),
        html.Div(id='histogram-container', children=[
            dcc.Graph(id='histogram-chart', style={'display': 'none'})
            ])
    ]),
    dcc.Interval(id='interval-component', interval=1000, n_intervals=0),  # Intervallo di aggiornamento ogni secondo
])"""

def add_message_processed(count):

    count=count+1
    return count
# initialize the data dictionary following the base config
baseconfig_path = "../data/ooqs/config.json"
block_array = []
count_message_processed = 0
return_dict = initialize_data_dict(baseconfig_path)
monitoring_socket = return_dict['monitoring_socket']
block_array = return_dict['block_array']

# Coda per la comunicazione tra thread
data_queue = Queue()

###### this is only for development
"""with open('../data/ooqs/WorkerManager-RTADP1-Rate.json', 'r') as f:
    new_dict = json.load(f)
    data_queue.put(new_dict)

with open('../data/ooqs/WorkerManager-RTADP1-S22Mean.json', 'r') as f:
    new_dict = json.load(f)
    data_queue.put(new_dict)

with open('../data/ooqs/WorkerManager-RTADP2-Rate.json', 'r') as f:
    new_dict = json.load(f)
    data_queue.put(new_dict)
"""

######

#start zmq monitor

# Funzione per la ricezione di messaggi da ZeroMQ
def zeromq_listener():
    context = zmq.Context()
    socket = context.socket(zmq.PULL)
    socket.connect(monitoring_socket)  # Esempio: connessione TCP su localhost, porta 5555

    print("connect##########")
    count = 0
    while True:
        msg = socket.recv_string()
        # Elabora il messaggio ricevuto (assumiamo che i dati siano nel formato corretto per il plot)
        # Qui potresti fare il parsing dei dati ricevuti da ZeroMQ e metterli nella coda
        # Qui c'è un esempio di come potrebbe essere aggiunto alla coda:
        
        count=count+1
        print("message received="+str(msg))
        data_queue.put(msg)

# Thread per ascoltare ZeroMQ
zeromq_thread = threading.Thread(target=zeromq_listener)
zeromq_thread.daemon = True  
zeromq_thread.start()



###########################################################################################################################
###########################################################################################################################
# Callback per rendere invisibile l'istogramma al primo accesso
"""
@app.callback(
    [Output('histogram-chart', 'style'),
     Output('hidehist-button', 'children')],
    [Input('hidehist-button', 'n_clicks'),
     Input('sunburst-chart', 'clickData'),
     Input('interval-component', 'n_intervals')],
    [State('histogram-chart', 'style'),
     State('hidehist-button', 'children')]
)
def update_histogram_visibility(n_clicks, click_data, n_intervals, histogram_style, button_text):
    if click_data == None:
        raise PreventUpdate 
    global prev_interval
    global prev_nclicks
    # print(prev_interval, n_intervals, prev_nclicks, n_clicks)
    if prev_interval > -1 and prev_interval < n_intervals:
        prev_interval = n_intervals
        prev_nclicks = 'none'
        return {'display': 'none'}, 'Mostra Istogramma'
    prev_interval = n_intervals
    # Verifica che l'istogramma venga plottato solo nelle foglie
    clicked_point = click_data.get('points', [])
    selected_path = clicked_point[0].get('currentPath', None)
    if (not clicked_point) or (not selected_path) or len((selected_path.split('/'))) <= 2:
        raise PreventUpdate
    # La visibilità dell'istogramma, dopo aver selezionato almeno un component, 
    #   va cambiata se e solo se viene cliccato il bottone
    if prev_nclicks != 'none' and str(n_clicks) == prev_nclicks:
        raise PreventUpdate
    else:
        prev_nclicks = n_clicks
    if button_text == "Mostra Istogramma":  # Se il pulsante è stato premuto un numero dispari di volte
        # Rendi visibile l'istogramma e cambia il testo del pulsante a "Nascondi Istogramma"
        histogram_style = {'display': 'block'}
        button_text = 'Nascondi Istogramma'
        prev_nclicks = str(n_clicks)
    else:
        # Altrimenti, rendi invisibile l'istogramma e cambia il testo del pulsante a "Mostra Istogramma"
        histogram_style = {'display': 'none'}
        button_text = 'Mostra Istogramma'
        prev_nclicks = 'none'
    # print('ciao')
    return histogram_style, button_text
    """
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
    [Input('interval-component', 'n_intervals')],
)
def update_sunburst(n_intervals):
    global prev_interval
    if n_intervals is None:
        raise PreventUpdate

    print("before queue size"+str( data_queue.qsize()))
    while not data_queue.empty():
    
        # Prova a prendere un messaggio dalla coda senza bloccare
        message = data_queue.get()
        global count_message_processed
        count_message_processed = count_message_processed+1
        print(f"Messaggio processato="+str(count_message_processed))
        update_block_array(block_array,json.loads(message))
        data_queue.task_done()
   
    print("after queue size"+str(data_queue.qsize()))
    data_dict = create_chart_input_from_dict(block_array)
    print(data_dict)
    sunburst_chart = draw_sunchart(data_dict)
    print('action: sunburstchart_updated')
    return sunburst_chart
###########################################################################################################################
###########################################################################################################################

# Esegui l'app
if __name__ == '__main__':
    host = '0.0.0.0'
    port = 8050
    app.run_server(host=host, port=port, debug=False,use_reloader=False)
