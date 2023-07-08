const HtmlWebpackPlugin = require('html-webpack-plugin')
var HtmlWebpackInlineSourcePlugin = require('html-webpack-inline-source-plugin');

module.exports = {
    entry: './index.js',
    output: {
        path: __dirname + '/dist',
        filename: 'index_bundle.js',
        publicPath: '/',
    },
    plugins: [
        new HtmlWebpackPlugin({
            publicPath: '/',
            template: "./index.html",
            inlineSource: '.(js|css)$'
        }),
        new HtmlWebpackInlineSourcePlugin(HtmlWebpackPlugin)
    ],
    mode: 'production'
}