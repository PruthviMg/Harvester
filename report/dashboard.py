# enhanced_dashboard.py
import pandas as pd
import numpy as np
import plotly.express as px
import plotly.graph_objects as go
from dash import Dash, dcc, html, Input, Output

# -----------------------------
# Load datasets
# -----------------------------
soil_df = pd.read_csv("soil_data.csv")
crop_df = pd.read_csv("simulation_output.csv")

# -----------------------------
# Merge soil and crop data for analysis
# -----------------------------
df = pd.merge(crop_df, soil_df, left_on=['TileX','TileY'], right_on=['x','y'])

# -----------------------------
# KPI Calculations
# -----------------------------
def compute_kpis(df):
    kpis = {}
    kpis['avg_growth'] = df['Growth'].mean()
    kpis['growth_std'] = df['Growth'].std()
    kpis['avg_time_to_mature'] = df['TimeToMature'].mean()
    kpis['land_utilization'] = (df['SoilQuality'] > 0.4).mean() * 100
    kpis['avg_soil_quality'] = df['soilBaseQuality'].mean()
    kpis['high_compaction_pct'] = (df['compaction'] > 0.7).mean() * 100
    kpis['high_salinity_pct'] = (df['salinity'] > 0.7).mean() * 100
    kpis['low_organic_matter_pct'] = (df['organicMatter'] < 0.2).mean() * 100
    return kpis

kpis = compute_kpis(df)

# -----------------------------
# Tile-level suggestions
# -----------------------------
def generate_suggestions(row):
    suggestions = []
    if row['soilBaseQuality'] < 0.4: suggestions.append("Add compost / improve fertility")
    elif row['soilBaseQuality'] > 0.7: suggestions.append("Ideal soil")
    if row['nutrients'] < 0.4: suggestions.append("Apply targeted fertilizer")
    elif row['nutrients'] > 0.7: suggestions.append("No extra fertilization needed")
    if row['pH'] < 0.4: suggestions.append("Apply lime to raise pH")
    elif row['pH'] > 0.6: suggestions.append("Consider acid-tolerant crops")
    if row['organicMatter'] < 0.2: suggestions.append("Add compost or cover crops")
    if row['compaction'] > 0.7: suggestions.append("Aerate or till soil")
    if row['salinity'] > 0.7: suggestions.append("Use salt-tolerant crops or leach soil")
    if row['sunlight'] < 0.4: suggestions.append("Consider shade-tolerant crops")
    # Growth efficiency alert
    if row['Growth'] < row['SoilQuality']*0.8: suggestions.append("Growth below expected; investigate limiting factors")
    return "; ".join(suggestions)

df['Suggestions'] = df.apply(generate_suggestions, axis=1)
df['GrowthEfficiency'] = df['Growth'] / df['SoilQuality']

# -----------------------------
# Correlation data
# -----------------------------
soil_features = ['soilBaseQuality','sunlight','nutrients','pH','organicMatter','compaction','salinity']
correlations = df[soil_features + ['Growth']].corr()['Growth'].sort_values(ascending=False).drop('Growth')

# -----------------------------
# Initialize Dashboard
# -----------------------------
app = Dash(__name__)

app.layout = html.Div([
    html.H1("Farming Report", style={'text-align': 'center'}),
    
    html.Div([
        html.H3("Key Performance Indicators (KPIs)"),
        html.Ul([
            html.Li(f"Average Crop Growth: {kpis['avg_growth']:.2f}"),
            html.Li(f"Growth Variability (Std Dev): {kpis['growth_std']:.2f}"),
            html.Li(f"Average Time to Mature: {kpis['avg_time_to_mature']:.1f}"),
            html.Li(f"Land Utilization (% tiles suitable): {kpis['land_utilization']:.1f}%"),
            html.Li(f"Average Soil Quality: {kpis['avg_soil_quality']:.2f}"),
            html.Li(f"High Compaction Tiles (%): {kpis['high_compaction_pct']:.1f}%"),
            html.Li(f"High Salinity Tiles (%): {kpis['high_salinity_pct']:.1f}%"),
            html.Li(f"Low Organic Matter Tiles (%): {kpis['low_organic_matter_pct']:.1f}%")
        ])
    ]),
    
    html.Hr(),
    
    html.H3("Interactive Soil / Growth Maps"),
    dcc.Dropdown(
        id='map-feature',
        options=[{'label': f, 'value': f} for f in soil_features + ['Growth', 'GrowthEfficiency']],
        value='soilBaseQuality'
    ),
    dcc.Graph(id='soil-map'),

    html.Hr(),

    html.H3("Tile-Level Suggestions"),
    dcc.Dropdown(
        id='tile-dropdown',
        options=[{'label': f"Tile ({row.TileX},{row.TileY}) - {row.CropName}", 'value': idx} 
                 for idx, row in df.iterrows()],
        value=0
    ),
    html.Div(id='tile-suggestion'),

    html.Hr(),
    html.H3("Crop-Specific Insights"),
    dcc.Dropdown(
        id='crop-dropdown',
        options=[{'label': c, 'value': c} for c in df['CropName'].unique()],
        value='Barley'
    ),
    dcc.Graph(id='crop-bar-chart'),

    html.Hr(),
    html.H3("Soil Feature Correlations with Growth"),
    dcc.Graph(
        figure=px.bar(x=correlations.index, y=correlations.values, labels={'x':'Soil Feature', 'y':'Correlation'}, title="Correlation of Soil Features with Growth")
    )
])

# -----------------------------
# Callbacks
# -----------------------------
@app.callback(
    Output('soil-map', 'figure'),
    Input('map-feature', 'value')
)
def update_map(feature):
    fig = px.scatter(
        df, x='TileX', y='TileY', color=feature, 
        size_max=12, color_continuous_scale='Viridis',
        title=f"{feature} Map"
    )
    fig.update_layout(yaxis=dict(autorange='reversed'))
    return fig

@app.callback(
    Output('tile-suggestion', 'children'),
    Input('tile-dropdown', 'value')
)
def show_tile_suggestion(tile_idx):
    row = df.iloc[tile_idx]
    return html.Ul([html.Li(s) for s in row['Suggestions'].split("; ")])

@app.callback(
    Output('crop-bar-chart', 'figure'),
    Input('crop-dropdown', 'value')
)
def update_crop_chart(crop_name):
    crop_df_filtered = df[df['CropName'] == crop_name]
    fig = px.bar(crop_df_filtered, x='TileX', y='Growth', color='SoilQuality',
                 labels={'TileX':'Tile X', 'Growth':'Growth', 'SoilQuality':'Soil Quality'},
                 title=f"Growth and Soil Quality for {crop_name}")
    return fig

# -----------------------------
# Run App
# -----------------------------
if __name__ == '__main__':
    app.run(debug=True)
